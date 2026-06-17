#include <kernel/sys.h>
#include <kernel/tty.h>

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

