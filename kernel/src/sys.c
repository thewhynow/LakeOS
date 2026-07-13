#include <kernel/sys.h>
#include <kernel/tty.h>
#include <kernel/kmm.h>
#include <kernel/exe.h>
#include <sys/sys.h>

void ISR_syscall_handler(registers_t *regs){
    switch(regs->eax){
        case SYSCALL_READ: {
            regs->eax = sys_read(regs->edi, (void*) regs->esi, regs->edx);
            break;
        }

        case SYSCALL_WRITE: {
            regs->eax = sys_write(regs->edi, (void*) regs->esi, regs->edx);
            break;
        }

        case SYSCALL_OPEN: {
            regs->eax = sys_open((void*) regs->edi, regs->esi);
            break;
        }

        case SYSCALL_CLOSE: {
            regs->eax = sys_close(regs->edi);
            break;
        }

        case SYSCALL_STAT: {
            regs->eax = sys_stat((void*) regs->edi, (void*) regs->esi);
            break;
        }

        case SYSCALL_FSTAT: {
            regs->eax = sys_fstat(regs->edi, (void*) regs->esi); 
            break;
        }

        case SYSCALL_EXEC: {
            regs->eax = sys_exec((void*) regs->edi, regs->esi, (void*) regs->edx);
            break;
        }

        case SYSCALL_EXIT: {
            sys_exit(regs->edi);
            __builtin_unreachable();
        }

        default: {
            regs->eax = -1;
        }
    }
}

size_t sys_read(int fd, void *buff, size_t count){
    return VFS_read(fd, buff, count);
}

size_t sys_write(int fd, const void *buff, size_t count){
    return VFS_write(fd, buff, count);
}

int sys_open(const char *filename, uint8_t mode){
    return VFS_open(filename, mode);
}

int sys_close(int fd){
    VFS_close(fd);
    return 0;
}

int sys_stat(const char *filename, t_FileStat *statbuff){
    VFS_stat(filename, statbuff);
    return 0;
}

int sys_fstat(int fd, t_FileStat *statbuff){
    VFS_fstat(fd, statbuff);
    return 0;
}

size_t sys_lseek(int fd, ssize_t offset, int whence){
    return VFS_seek(fd, offset, whence);
}

void *sys_mmap(void *addr, size_t length, int prot, int flags, int fd, size_t offset){
    if ((uint32_t) addr % 0x1000)
        return NULL;

    /* round up to page boundary */
    uint32_t round_length = (length + 0xFFF) & ~0xFFFU;

    void *res = NULL;
    /* exe.c */
    extern t_Process *process_stack;

    if (flags & SYSCALL_MMAP_FLAG_FIXED){
        umap_pages(process_stack, addr, round_length, prot, flags);
        res = addr;
    }
    else
        res = ualloc_pages(process_stack, round_length, prot, flags);

    if (res == NULL)
        return (void*) -1;

    if (flags & SYSCALL_MMAP_FLAG_ANONYMOUS)
        memset(res, 0, round_length);
    else {
        size_t curr_off = sys_lseek(fd, 0, SYSCALL_LSEEK_CUR);

        if (sys_lseek(fd, offset, SYSCALL_LSEEK_SET) != offset)
            goto fail;

        size_t bytes_read = sys_read(fd, res, length);

        memset(res + bytes_read, 0, length - bytes_read);

        if (sys_lseek(fd, curr_off, SYSCALL_LSEEK_SET) != curr_off)
            goto fail;
    }

    return res;

fail:
    umm_unmap_pages(process_stack, res);
    return (void*) -1;
}

int sys_exec(const char *path, int argc, char **argv){
    stat_t file_stat;
    stat(path, &file_stat);

    void *buff = kmalloc(file_stat.size);
    int fd = open(path, VFS_FILE_READ);
    read(fd, buff, file_stat.size);
    close(fd);

    execute(buff, argc, argv); 
}

void sys_exit(int status){
    exit_process(status);  
}
