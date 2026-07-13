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

int sys_lseek(int fd, size_t offset, int whence){
    return VFS_seek(fd, offset, whence);
}

void *sys_mmap(void *addr, size_t length, int prot, int flags, int fd, size_t offset){
    /* addr parameter is ignored - assume NULL */

    size_t num_pages = ((length + (0x1000 - 1)) / 0x1000);


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
