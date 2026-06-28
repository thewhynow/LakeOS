.section .text
.global jump_ring3
jump_ring3:
  movl 4(%esp), %esp

  popl %eax
  movw %cx, %ds
  movw %cx, %es
  movw %cx, %fs
  movw %cx, %gs

  /* set up stack frame iret expects */
  popa
  addl $8, %esp
  iret

