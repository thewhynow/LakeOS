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

        case SYSCALL_EXEC: {
            regs->eax = sys_exec((void*) regs->edi, (void*) regs->esi, (void*) regs->edx);
            break;
        }

        case SYSCALL_EXIT: {
            regs->eax = regs->edi;
            sys_exit(regs->edi);
            break;
        }

        default: {
            regs->eax = -1;
        }
    }
}

size_t sys_read(int fd, void *buff, size_t count){
    return VFS_read((void*) fd, buff, count);
}

size_t sys_write(int fd, const void *buff, size_t count){
    if (fd == 1)
        terminal_write(buff, count);
    else
        return VFS_write((void*) fd, buff, count);
}

int sys_open(const char *filename, uint8_t mode){
    return (int) VFS_open(filename, mode);
}

int sys_close(int fd){
    VFS_close((void*) fd);
    return 1;
}

int sys_stat(const char *filename, const t_FileStat *statbuff){
    VFS_stat(filename, statbuff);
    return 1;
}

int sys_exec(const char *path, char *const *argv, char *const *envp){
    stat_t file_stat;
    stat(path, &file_stat);

    void *buff = kmalloc(file_stat.size);
    int fd = open(path, VFS_FILE_READ);
    read(fd, buff, file_stat.size);

    execute(buff, 0, NULL); 
}

void sys_exit(int status){

}
