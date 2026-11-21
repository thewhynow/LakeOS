#include <kernel/sys.h>
#include <kernel/tty.h>
#include <kernel/kmm.h>
#include <kernel/exe.h>
#include <sys/syscall.h>

void ISR_syscall_handler(registers_t *regs){
    /* interrupt gate clears IF; re-enable so PIT ticks advance (PIT_sleep) */
    asm volatile("sti");

    switch(regs->eax){
        case SYSCALL_READ: {
            regs->eax = sys_read(regs->ebx, (void*) regs->ecx, regs->edx);
            break;
        }

        case SYSCALL_WRITE: {
            regs->eax = sys_write(regs->ebx, (void*) regs->ecx, regs->edx);
            break;
        }

        case SYSCALL_OPEN: {
            regs->eax = sys_open((void*) regs->ebx, regs->ecx);
            break;
        }

        case SYSCALL_CLOSE: {
            regs->eax = sys_close(regs->ebx);
            break;
        }

        case SYSCALL_STAT: {
            regs->eax = sys_stat((void*) regs->ebx, (void*) regs->ecx);
            break;
        }

        case SYSCALL_FSTAT: {
            regs->eax = sys_fstat(regs->ebx, (void*) regs->ecx); 
            break;
        }

        case SYSCALL_LSEEK: {
            regs->eax = sys_lseek(regs->ebx, regs->ecx, regs->edx);
            break;
        }

        case SYSCALL_MMAP: {
            regs->eax = 
                (uint32_t) sys_mmap(
                    (void*) regs->ebx, regs->ecx, 
                    regs->edx, regs->esi, 
                    regs->edi, regs->ebp
                );
            break;
        }

        case SYSCALL_MPROTECT: {
            regs->eax = sys_mprotect((void*) regs->ebx, regs->ecx, regs->edx);
            break;
        }

        case SYSCALL_MUNMAP: {
            regs->eax = sys_munmap((void*) regs->ebx, regs->ecx);
            break;
        }

        case SYSCALL_MSYNC: {
            regs->eax = sys_msync((void*) regs->ebx, regs->ecx, regs->edx);
            break;
        }

        case SYSCALL_EXEC: {
            regs->eax = sys_exec((void*) regs->ebx, regs->ecx, (void*) regs->edx);
            break;
        }

        case SYSCALL_EXIT: {
            sys_exit(regs->ebx);
            __builtin_unreachable();
        }

        default: {
            regs->eax = -1;
        }
    }

    asm volatile("cli");
}

size_t sys_read(int fd, void *buff, size_t count){
    return VFS_read(fd, buff, count);
}

size_t sys_write(int fd, const void *buff, size_t count){
    return VFS_write(fd, buff, count);
}

int sys_open(const char *filename, uint8_t mode){
    int fd = VFS_open(filename, mode);

    if (fd == -1 && mode & SYSCALL_OPEN_CREAT){
        VFS_create(filename, 0);
        fd = VFS_open(filename, mode);
    }

    return fd;
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
        return (void*)-1;

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
    else
        umm_update_mapping(process_stack, addr, round_length, prot, fd, offset);

    return res;

fail:
    umm_unmap_pages(process_stack, res);
    return (void*) -1;
}

int sys_munmap(void *addr, size_t length){
    if ((uint32_t) addr % 0x1000)
        return -1;

    /* round up to page boundary */
    uint32_t round_length = (length + 0xFFF) & ~0xFFFU;

    /* exe.c */
    extern t_Process *process_stack;
    umm_unmap_range(process_stack, addr, round_length);

    return 0;
}

int sys_msync(void *addr, size_t length, int flags){
    /* exe.c */
    extern t_Process *process_stack;

    if (flags & SYSCALL_MSYNC_INVALIDATE)
        return -1;

    umm_sync_file(process_stack, addr, length);

    return 0;
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
