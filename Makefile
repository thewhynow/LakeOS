# ============================================================================
#  LakeOS build
# ============================================================================
#  Targets:
#    make            build lakeos.iso
#    make run        build + launch qemu
#    make clean      remove build artifacts
#
#  Options for `make run` (all optional):
#    DEBUG=1         launch qemu paused for GDB (-s -S)
#                    connect with:  gdb iso/boot/lakeos.elf
#                                   (gdb) target remote localhost:1234
#    LOG=1           log CPU state on interrupt (-d int -D qemu.log)
#    FD=N            create N blank 1.44MB floppies (index 0..N-1)
#    HD=N            create N blank 1MB hard disks  (index 1..N; boot uses 0)
#    FATFD=1         create + FAT-format a floppy   (fat.img)
#    FATHD=1         create + FAT-format a hard disk (fat.img)
#    REGULAR=1       mount the persistent system.img as an IDE disk
#
#  e.g.  make run DEBUG=1 LOG=1
#        make run FATFD=1
# ============================================================================

UNAME_S := $(shell uname)
ifeq ($(UNAME_S),Darwin)
  CC            := i686-elf-gcc
  AS            := i686-elf-as
  GRUB_MKRESCUE := i686-elf-grub-mkrescue
else
  CROSS         := /home/thewhynow/opt/cross/bin
  CC            := $(CROSS)/i686-elf-gcc
  AS            := $(CROSS)/i686-elf-as
  GRUB_MKRESCUE := grub-mkrescue
endif

CFLAGS  := -std=gnu99 -ffreestanding -D__is_libk -nostdlib -O0 -w \
           -Ikernel/include -Ilibc/include -g -MMD -MP
ASFLAGS :=
LDFLAGS := -T kernel/linker.ld -ffreestanding -nostdlib -O0 -w -g
LDLIBS  := -lgcc

BUILD   := build
C_SRCS  := $(wildcard kernel/src/*.c) $(wildcard libc/*/*.c)
AS_SRCS := $(wildcard kernel/asm/*.s)
OBJS    := $(patsubst %,$(BUILD)/%.o,$(basename $(C_SRCS) $(AS_SRCS)))

CRTI      := $(BUILD)/kernel/asm/crti.o
CRTN      := $(BUILD)/kernel/asm/crtn.o
LINK_LIST := $(CRTI) $(filter-out $(CRTI) $(CRTN),$(OBJS)) $(CRTN)

KERNEL := iso/boot/lakeos.elf
ISO    := lakeos.iso

.PHONY: all run clean
all: $(ISO)

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS)

$(BUILD)/%.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $< -o $@ $(ASFLAGS)

$(KERNEL): $(OBJS) kernel/linker.ld
	$(CC) $(LDFLAGS) -o $@ $(LINK_LIST) $(LDLIBS)

$(ISO): $(KERNEL)
	$(GRUB_MKRESCUE) -o $@ iso

-include $(OBJS:.o=.d)

FD ?= 0
HD ?= 0

run: $(ISO)
	@qf="-m 512 -cdrom $(ISO) -boot d"; \
	fi=0; hi=1; \
	i=0; while [ $$i -lt $(FD) ]; do \
	  dd if=/dev/zero of=floppy$$fi.img bs=512 count=2880 2>/dev/null; \
	  qf="$$qf -drive file=floppy$$fi.img,format=raw,index=$$fi,if=floppy"; \
	  fi=$$((fi+1)); i=$$((i+1)); \
	done; \
	i=0; while [ $$i -lt $(HD) ]; do \
	  dd if=/dev/zero of=harddisk$$hi.img bs=1M count=1 2>/dev/null; \
	  qf="$$qf -drive file=harddisk$$hi.img,format=raw,index=$$hi,if=ide"; \
	  hi=$$((hi+1)); i=$$((i+1)); \
	done; \
	if [ -n "$(FATFD)" ]; then \
	  dd if=/dev/zero of=fat.img bs=512 count=2880 2>/dev/null; mformat -i fat.img ::; \
	  qf="$$qf -drive file=fat.img,format=raw,index=$$fi,if=floppy"; fi=$$((fi+1)); \
	fi; \
	if [ -n "$(FATHD)" ]; then \
	  dd if=/dev/zero of=fat.img bs=1M count=1 2>/dev/null; mformat -i fat.img ::; \
	  qf="$$qf -drive file=fat.img,format=raw,index=$$hi,if=ide"; hi=$$((hi+1)); \
	fi; \
	if [ -n "$(REGULAR)" ]; then \
	  qf="$$qf -drive file=system.img,format=raw,index=$$hi,if=ide"; hi=$$((hi+1)); \
	fi; \
	if [ -n "$(LOG)" ]; then qf="$$qf -d int -D qemu.log"; fi; \
	if [ -n "$(DEBUG)" ]; then qf="$$qf -s -S"; fi; \
	echo "$$qf"; \
	qemu-system-i386 $$qf; \
	i=0; while [ $$i -lt $(FD) ]; do rm -f floppy$$i.img; i=$$((i+1)); done; \
	i=1; while [ $$i -le $(HD) ]; do rm -f harddisk$$i.img; i=$$((i+1)); done

clean:
	rm -rf $(BUILD) $(KERNEL) $(ISO) qemu.log
