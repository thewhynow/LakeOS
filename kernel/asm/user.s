.section .text
.global jump_ring3
jump_ring3:
  movl 4(%esp), %esp

  popl %eax
  movw %ax, %ds
  movw %ax, %es
  movw %ax, %fs
  movw %ax, %gs

  /* set up stack frame iret expects */
  popa
  addl $8, %esp
  iret

