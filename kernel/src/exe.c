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

void *add_entrance_args(void *stack_base, int argc, const char **argv){
    char **temp_argv = kmalloc(sizeof(char*) * (argc + 1));
    temp_argv[argc] = NULL;

    for (int i = 0; i < argc; ++i){
        int len = strlen(argv[i]);
        stack_base -= len + 1;
        temp_argv[i] = strcpy(stack_base, argv[i]);
    }

    stack_base 
        = memcpy((char**) stack_base - argc - 1, temp_argv, sizeof(char*) * (argc + 1)); 

    stack_base -= sizeof(int);
    *(int*) stack_base = argc;

    kfree(temp_argv);
    return stack_base;
}

void *new_stack(int argc, const char **argv){
    void *page = (void*) 0xC0000000;

    for (int i = 0; i < 64; ++i){
        page -= 0x1000;
        valloc_page(page);
    }

    void *stack_base 
        = add_entrance_args((void*) 0xC0000000, argc, argv);

    return stack_base;
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
    vmm_map_page(process_stack->address_space, curr_page_directory, false);

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

int execute(const void *file_buff, int argc, char **argv){
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
