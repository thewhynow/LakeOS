.section .text

.extern isr_return

/* void enter_context(registers_t registers) */
.global enter_context
enter_context:
    movl 4(%esp), %esp /* esp = &registers */ 
    jmp isr_return

