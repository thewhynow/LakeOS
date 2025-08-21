#!/bin/bash

##########################################################################
#   BUILDS AND RUNS THE OPERATING SYSTEM USING QEMU                      #
#                                                                        #
#   FLAGS:                                                               #
#       hd (up to 3)                                                     #
#             create a 1MB harddrive image initialized to 0              #
#             virtually mount it on the next available index             #
#             first HD slot is used by boot disk                         #
#       fd (up to 2)                                                     #
#             create a 1.44MB 3.5" floppy-disk image initialized to 0    #
#             virtually mount it on the next available drive             #
#       debug                                                            #
#             build with debug flags and launch qemu on pause            #
#             qemu pauses waiting for GDB server connection              #
#       debug-fat                                                        #
#             create a 1.44MB 3.5" floppy-disk image initialized to 0    #
#             format it as a FAT filesystem                              #
#             virtually mount the image on the next available drive      #
#       log                                                              #
#             pass flags to qemu to log CPU state when interrupt raised  #
##########################################################################

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

x_flags="-std=c++98 -ffreestanding -D_KERNEL_LIBC -nostdlib -O0 -w -g"
c_flags="-std=gnu99 -ffreestanding -D_KERNEL_LIBC -nostdlib -O0 -w -g"
s_flags=""
q_flags=" -m 512"
q_flags+=" -cdrom lakeos.iso -boot d"

f_index=0

# start at 1 since boot disk uses drive 0 #
h_index=1

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
            dd if=/dev/zero of=floppy${f_index}.img bs=512 count=2880
            q_flags+=" -drive file=floppy${f_index}.img,format=raw,index=${f_index},if=floppy"
            ((f_index++))
            ;;
        "hd")
            dd if=/dev/zero of=harddisk${h_index}.img bs=1M count=1
            q_flags+=" -drive file=harddisk${h_index}.img,format=raw,index=${h_index},if=ide"
            ((h_index++))
            ;;
        "debug-fat-hd")
            dd if=/dev/zero of=fat.img bs=1M count=1
            mformat -i fat.img ::
            q_flags+=" -drive file=fat.img,format=raw,index=${h_index},if=ide"
            ((h_index++))
            ;;
        "debug-fat-fd")
            dd if=/dev/zero of=fat.img bs=512 count=2880
            mformat -i fat.img ::
            q_flags+=" -drive file=fat.img,format=raw,index=${f_index},if=floppy"
            ((f_index++))
    esac
done

$compiler_path -c kernel/src/kernel.c     -o kernel.o     $c_flags
$compiler_path -c kernel/src/tty.c        -o tty.o        $c_flags
$compiler_path -c kernel/src/gdt.c        -o gdt.o        $c_flags
$compiler_path -c kernel/src/irq.c        -o irq.o        $c_flags
$compiler_path -c kernel/src/idt.c        -o idt.o        $c_flags
$compiler_path -c kernel/src/isr.c        -o isr.o        $c_flags
$compiler_path -c kernel/src/io.c         -o io.o         $c_flags
$compiler_path -c kernel/src/pic.c        -o pic.o        $c_flags
$compiler_path -c kernel/src/pit.c        -o pit.o        $c_flags
$compiler_path -c kernel/src/ps2.c        -o ps2.o        $c_flags
$compiler_path -c kernel/src/multiboot.c  -o multiboot.o  $c_flags
$compiler_path -c kernel/src/pmm.c        -o pmm.o        $c_flags
$compiler_path -c kernel/src/vmm.c        -o vmm.o        $c_flags
$compiler_path -c kernel/src/dma.c        -o dma.o        $c_flags
$compiler_path -c kernel/src/fdc.c        -o fdc.o        $c_flags
$compiler_path -c kernel/src/ata.c        -o ata.o        $c_flags
$compiler_path -c kernel/src/sal.c        -o sal.o        $c_flags
$compiler_path -c kernel/src/fat.c        -o fat.o        $c_flags
$compiler_path -c kernel/src/kmm.c        -o kmm.o        $c_flags
$compiler_path -c kernel/src/rtc.c        -o rtc.o        $c_flags

$compiler_path -c libc/stdio/printf.c     -o printf.o     $c_flags
$compiler_path -c libc/stdio/putchar.c    -o putchar.o    $c_flags
$compiler_path -c libc/stdio/puts.c       -o puts.o       $c_flags
$compiler_path -c libc/stdio/getchar.c    -o getchar.o    $c_flags
$compiler_path -c libc/stdio/gets.c       -o gets.o       $c_flags

$compiler_path -c libc/stdlib/abort.c     -o abort.o      $c_flags

$compiler_path -c libc/string/memcmp.c    -o memcmp.o     $c_flags
$compiler_path -c libc/string/memcpy.c    -o memcpy.o     $c_flags
$compiler_path -c libc/string/memmove.c   -o memmove.o    $c_flags
$compiler_path -c libc/string/memset.c    -o memset.o     $c_flags
$compiler_path -c libc/string/strlen.c    -o strlen.o     $c_flags
$compiler_path -c libc/string/strncat.c   -o strncat.o    $c_flags
$compiler_path -c libc/string/strchr.c    -o strchr.o     $c_flags
$compiler_path -c libc/string/strtok.c    -o strtok.o     $c_flags
$compiler_path -c libc/string/strpbrk.c   -o strpbrk.o    $c_flags


$assemble_path kernel/asm/boot.s          -o boot.o       $s_flags
$assemble_path kernel/asm/crti.s          -o crti.o       $s_flags
$assemble_path kernel/asm/crtn.s          -o crtn.o       $s_flags
$assemble_path kernel/asm/gdt.s           -o _gdt.o       $s_flags
$assemble_path kernel/asm/idt.s           -o _idt.o       $s_flags
$assemble_path kernel/asm/isr.s           -o _isr.o       $s_flags
$assemble_path kernel/asm/irq.s           -o _irq.o       $s_flags

$compiler_path -T kernel/linker.ld -o iso/boot/lakeos.elf $c_flags -lgcc *.o

rm *.o

$grub_iso_path -o lakeos.iso iso
echo $q_flags
qemu-system-i386 $q_flags

rm lakeos.iso
rm iso/boot/lakeos.elf

for ((i=0; i<f_index; i++)); do
    rm floppy${i}.img
done

for ((i=1; i<h_index; i++)); do
    rm harddisk${i}.img
done
