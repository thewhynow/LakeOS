/**
 * @file exe.h
 * @brief processes, there is no scheduler: they form a stack and
 *  exec pushes onto it, exit pops it
 */

#include <kernel/elf.h>
#include <kernel/pmm.h>
#include <kernel/vmm.h>
#include <kernel/kmm.h>
#include <kernel/isr.h>
#include <kernel/umm.h>

#ifndef __EXE_H
#define __EXE_H

typedef struct t_Process t_Process;

/**
 * a userspace process
 */
struct t_Process {
    /** registers it gets entered with, user segments and all */
    registers_t  context;
    /** 2 pages, esp0 in the tss points to the top of it while this runs */
    void *kernel_stack;
    pdirectory_t *address_space;
    /** its mappings, sorted by address (see umm.h) */
    umm_block_t *blocks;
    /** the process that exec'd this one, gets resumed on exit */
    t_Process *prev;
};

/**
 * @brief runs an elf as a new process and jumps into it, doesn't return
 *  frees file_buff (so it has to be kmalloc'd)
 * @param file_buff the whole elf file in memory
 * @param argc argument count
 * @param argv arguments, they get copied onto the new process's stack
 * @return nothing really, never gets there
 */
int execute(const void *file_buff, int argc, char **argv);

/**
 * @brief kills the current process and resumes the one that exec'd it
 *  with code as the return value of its exec syscall
 * @param code the exit status
 */
void exit_process(int code);

/**
 * @brief makes a new page directory, see vmm.h
 * @return the new directory
 */
pdirectory_t *new_page_directory();

/**
 * entry point of a process
 */
typedef void (*f_entry)(void);

/**
 * @brief makes a process with a fresh address space and stack and
 *  pushes it on the process stack, switching to its page directory
 *  the elf still has to be loaded into it
 * @param argc argument count
 * @param argv arguments
 * @return the new process
 */
t_Process *new_process(int argc, char **argv);

/**
 * @brief maps 64 pages below 0xC0000000 for a stack and puts the args on it
 * @param argc argument count
 * @param argv arguments
 * @return the initial esp for the process
 */
void *new_stack(int argc, const char **argv);

/**
 * @brief copies argv onto the stack and sets it up the way
 *  the entry point expects it: argc on top, then argv, then
 *  the null at the end of argv
 * @param stack_base the top of the stack, it grows down from here
 * @param argc argument count
 * @param argv arguments
 * @return the new stack pointer
 */
void *add_entrance_args(
    void *stack_base,
    int argc,
    const char **argv
);

#endif
