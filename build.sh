if [[ $(uname) == "Darwin" ]]; then
    compiler_path="i686-elf-gcc"
    cxx_cmpl_path="i686-elf-g++"
    assemble_path="i686-elf-as"
    grub_iso_path="i686-elf-grub-mkrescue"
else
    compiler_path="/home/thewhynow/opt/cross/bin/i686-elf-gcc"
    cxx_cmpl_path="/home/thewhynow/opt/cross/bin/i686-elf-g++"
    assemble_path="/home/thewhynow/opt/cross/bin/i686-elf-as"
    grub_iso_path="grub-mkrescue"
fi

x_flags="-std=c++98 -ffreestanding -Wall -Wextra -D_KERNEL_LIBC -nostdlib"
c_flags="-std=gnu99 -ffreestanding -Wall -Wextra -D_KERNEL_LIBC -nostdlib"
s_flags=""
q_flags=" -m 512"

for arg in $@; do
    case $arg in
        "debug")
            c_flags+=" -g"
            s_flags+=" -g"
            x_flags+=" -g"
            q_flags+=" -s -S"
            ;;
        "log")
            q_flags+=" -d int -D qemu.log"
            ;;
        "fd")
            q_flags+=" -drive file=floppy.img,format=raw,index=0,if=floppy"
            ;;
        "hd")
            q_flags+=" -drive file=harddisk.img,format=raw,index=0"
            ;;
    esac
done

$compiler_path -c kernel/src/kernel.c        -o kernel.o    $c_flags
$compiler_path -c kernel/src/tty.c        -o tty.o       $c_flags
$compiler_path -c kernel/src/gdt.c        -o gdt.o       $c_flags
$compiler_path -c kernel/src/irq.c        -o irq.o       $c_flags
$compiler_path -c kernel/src/idt.c        -o idt.o       $c_flags
$compiler_path -c kernel/src/isr.c        -o isr.o       $c_flags
$compiler_path -c kernel/src/io.c         -o io.o        $c_flags
$compiler_path -c kernel/src/pic.c        -o pic.o       $c_flags
$compiler_path -c kernel/src/pit.c        -o pit.o       $c_flags
$compiler_path -c kernel/src/ps2.c        -o ps2.o       $c_flags
$compiler_path -c kernel/src/multiboot.c  -o multiboot.o $c_flags
$compiler_path -c kernel/src/pmm.c        -o pmm.o       $c_flags
$compiler_path -c kernel/src/vmm.c        -o vmm.o       $c_flags
$compiler_path -c kernel/src/dma.c        -o dma.o       $c_flags
$compiler_path -c kernel/src/fdc.c        -o fdc.o       $c_flags
$compiler_path -c kernel/src/ata.c        -o ata.o       $c_flags

$compiler_path -c libc/stdio/printf.c           -o printf.o    $c_flags
$compiler_path -c libc/stdio/putchar.c          -o putchar.o   $c_flags
$compiler_path -c libc/stdio/puts.c             -o puts.o      $c_flags
$compiler_path -c libc/stdio/getchar.c          -o getchar.o   $c_flags
$compiler_path -c libc/stdio/gets.c             -o gets.o      $c_flags

$compiler_path -c libc/stdlib/abort.c           -o abort.o     $c_flags

$compiler_path -c libc/string/memcmp.c          -o memcmp.o    $c_flags
$compiler_path -c libc/string/memcpy.c          -o memcpy.o    $c_flags
$compiler_path -c libc/string/memmove.c         -o memmove.o   $c_flags
$compiler_path -c libc/string/memset.c          -o memset.o    $c_flags
$compiler_path -c libc/string/strlen.c          -o strlen.o    $c_flags
$compiler_path -c libc/string/strncat.c         -o strncat.o   $c_flags

$assemble_path kernel/asm/boot.s      -o boot.o      $s_flags
$assemble_path kernel/asm/crti.s      -o crti.o      $s_flags
$assemble_path kernel/asm/crtn.s      -o crtn.o      $s_flags
$assemble_path kernel/asm/gdt.s       -o _gdt.o      $s_flags
$assemble_path kernel/asm/idt.s       -o _idt.o      $s_flags
$assemble_path kernel/asm/isr.s       -o _isr.o      $s_flags
$assemble_path kernel/asm/irq.s       -o _irq.o      $s_flags

$compiler_path -T kernel/linker.ld -o iso/boot/lakeos.elf $c_flags -lgcc *.o

rm *.o

$grub_iso_path -o lakeos.iso iso
qemu-system-i386 -cdrom lakeos.iso $q_flags

rm lakeos.iso
rm iso/boot/lakeos.elf