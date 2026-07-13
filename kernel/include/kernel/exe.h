#include <kernel/elf.h>
#include <kernel/pmm.h>
#include <kernel/vmm.h>
#include <kernel/kmm.h>
#include <kernel/isr.h>

#ifndef __EXE_H
#define __EXE_H

typedef struct t_Process t_Process;

struct t_Process {
    registers_t  context;
    void *kernel_stack;
    pdirectory_t *address_space;
    t_Process *prev;
};

int execute(const void *file_buff, int argc, char **argv);

void exit_process(int code);

pdirectory_t *new_page_directory();

typedef void (*f_entry)(void);

t_Process *new_process(int argc, char **argv);

void *new_stack(int argc, const char **argv);

void *add_entrance_args(
    void *stack_base, 
    int argc, 
    const char **argv
);

#endif
