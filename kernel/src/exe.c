#include <kernel/exe.h>

static t_Process *process_stack;

pdirectory_t *new_page_directory(){
    pdirectory_t *new_pd = valloc_page(NULL);
    memset(new_pd, 0, sizeof *new_pd);

    for (int i = 768; i < 1024; ++i)
        new_pd->entries[i] = kernel_page_directory.entries[i];

    vmm_unmap_page(new_pd);

    return new_pd;
}

void *new_stack(int argc, const char **argv){
    /* ignoring argv and argc for now cause i have to somehow map argv in */

    void *stack_begin = (void*) (0xC0000000 - 0x1000);

    for (int i = 0; i < 64; ++i){
        valloc_page(stack_begin);
        stack_begin -= 0x1000;
    }

    return (void*) 0xC0000000;
}

t_Process *new_process(){
    t_Process *new_proc = kmalloc(sizeof(t_Process));

    new_proc->address_space = new_page_directory();

    if (process_stack)
        new_proc->prev = process_stack;
    else
        new_proc->prev = NULL;

    process_stack = new_proc;

    switch_pd(process_stack->address_space);
    vmm_map_page(process_stack->address_space, &kernel_page_directory, false);

    /* TEMP */
    new_proc->context = (registers_t){0};

    new_proc->context = (registers_t){
        .ds     = (4 * 8) | 3,
        .cs     = (3 * 8) | 3,
        .eflags = 0x202,
        .esp    = (uint32_t) new_stack(0, NULL),
        .ss     = (4 * 8) | 3
    };
}

int execute(const void *file_buff, int argc, const char **argv){
    new_process();

    f_entry entry = elf_load(file_buff);

    process_stack->context.eip = (uint32_t) entry;

    /* kernel/asm/user.s */
    extern void jump_ring3(registers_t *regs); 
    jump_ring3(&process_stack->context);
}

int execute_execution(int code){
    vfree_page(process_stack->address_space);

    t_Process *prev = process_stack->prev;
    kfree(process_stack);
    process_stack = prev;
    
    /* TODO */
}
