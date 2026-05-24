.section .text
.global flush_tss
/* void flush_tss(void) */
flush_tss:
  movw $(5 * 8) | 0, %ax
  ltr %ax
  ret
