if [[ $(uname) == "Darwin" ]]; then
    compiler_path="i686-elf-gcc"
    assemble_path="i686-elf-as"
else
    compiler_path="/home/thewhynow/opt/cross/bin/i686-elf-gcc"
    assemble_path="/home/thewhynow/opt/cross/bin/i686-elf-as"
fi

c_flags="-std=gnu99 -ffreestanding -Wall -Wextra"

$compiler_path -c kernel/kernel/kernel.c        -o kernel.o   $c_flags
$compiler_path -c kernel/arch/i386/tty.c        -o tty.o      $c_flags
$compiler_path -c kernel/arch/i386/gdt.c        -o gdt.o      $c_flags
$compiler_path -c kernel/arch/i386/irq.c        -o irq.o      $c_flags
$compiler_path -c kernel/arch/i386/idt.c        -o idt.o      $c_flags
$compiler_path -c kernel/arch/i386/isr.c        -o isr.o      $c_flags
$compiler_path -c kernel/arch/i386/io.c         -o io.o       $c_flags
$compiler_path -c kernel/arch/i386/pic.c        -o pic.o      $c_flags

$compiler_path -c libc/stdio/printf.c           -o printf.o   $c_flags
$compiler_path -c libc/stdio/putchar.c          -o putchar.o  $c_flags
$compiler_path -c libc/stdio/puts.c             -o puts.o     $c_flags

$compiler_path -c libc/stdlib/abort.c           -o abort.o    $c_flags

$compiler_path -c libc/string/memcmp.c          -o memcmp.o   $c_flags
$compiler_path -c libc/string/memcpy.c          -o memcpy.o   $c_flags
$compiler_path -c libc/string/memmove.c         -o memmove.o  $c_flags
$compiler_path -c libc/string/memset.c          -o memset.o   $c_flags
$compiler_path -c libc/string/strlen.c          -o strlen.o   $c_flags

$assemble_path kernel/arch/i386/asm/boot.s      -o boot.o
$assemble_path kernel/arch/i386/asm/crti.s      -o crti.o
$assemble_path kernel/arch/i386/asm/crtn.s      -o crtn.o
$assemble_path kernel/arch/i386/asm/gdt.s       -o crtn.o
$assemble_path kernel/arch/i386/asm/idt.s       -o _idt.o
$assemble_path kernel/arch/i386/asm/isr.s       -o _isr.o
$assemble_path kernel/arch/i386/asm/irq.s       -o _irq.o

$compiler_path -T kernel/arch/i386/linker.ld    -o lakeos.bin $c_flags -lgcc -nostdlib *.o

rm *.o

qemu-system-i386 -kernel lakeos.bin

rm lakeos.bin