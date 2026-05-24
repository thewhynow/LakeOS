.section .text
.global user
user:
  movl $1, %eax        /* SYSCALL_WRITE */
  movl $1, %edi        
  movl $msg, %esi      /* buffer */
  movl $14, %edx       /* count */
  int $0x80
  1:  jmp 1b               /* spin so we don't fall off into garbage */
  msg: .asciz "hello ring3!\n"

.global jump_ring3
jump_ring3:
  movw $(4 * 8) | 3, %cx
  movw %cx, %ds
  movw %cx, %es
  movw %cx, %fs
  movw %cx, %gs

  movl $0xBFFFF000, %eax

  /* set up stack frame iret expects */
  pushl $(4 * 8) | 3
  pushl %eax
  pushf
  pushl $(3 * 8) | 3
  pushl $user
  iret
