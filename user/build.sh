i686-elf-as user.s -o user.o
i686-elf-ld -T user.ld -o INIT.ELF user.o

