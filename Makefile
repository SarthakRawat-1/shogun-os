CC := gcc
AS := as
LD := ld
OBJCOPY := objcopy

I686_CC := $(shell which i686-elf-gcc > /dev/null 2>&1 && echo i686-elf-gcc)
I686_AS := $(shell which i686-elf-as > /dev/null 2>&1 && echo i686-elf-as)
I686_LD := $(shell which i686-elf-ld > /dev/null 2>&1 && echo i686-elf-ld)

ifeq ($(I686_CC),i686-elf-gcc)
CC := $(I686_CC)
endif
ifeq ($(I686_AS),i686-elf-as)
AS := $(I686_AS)
endif
ifeq ($(I686_LD),i686-elf-ld)
LD := $(I686_LD)
endif

CFLAGS = -m32 -ffreestanding -fno-stack-protector -fno-builtin -nostdlib -nostartfiles
ASFLAGS = --32
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

SRCDIR = src
OBJDIR = obj
BINDIR = bin

BOOT = $(SRCDIR)/boot.s
KERNEL = $(SRCDIR)/kernel.c
TERMINAL = $(SRCDIR)/terminal.c
LIBC = $(SRCDIR)/libc.c
MEMORY = $(SRCDIR)/memory.c
IO = $(SRCDIR)/io.c
PORT_MANAGER = $(SRCDIR)/port_manager.c
RTC = $(SRCDIR)/rtc.c
GDT_C = $(SRCDIR)/gdt.c
GDT_S = $(SRCDIR)/gdt.s
IDT_C = $(SRCDIR)/idt.c
IDT_S = $(SRCDIR)/idt.s
LOGGER = $(SRCDIR)/logger.c
TEST = $(SRCDIR)/test.c
LINKER = linker.ld
TARGET_KERNEL = $(BINDIR)/kernel

.PHONY: all clean debug

all:
	mkdir -p $(OBJDIR) $(BINDIR)
	# Generate interrupt handlers assembly file
	python3 generate_interrupt_handlers.py
	# Generate IDT initialization code
	python3 generate_idt_init.py
	# Generate interrupt handler extern declarations
	python3 generate_interrupt_externs.py
	$(AS) $(ASFLAGS) $(BOOT) -o $(OBJDIR)/boot.o
	$(AS) $(ASFLAGS) $(GDT_S) -o $(OBJDIR)/gdt.o
	$(AS) $(ASFLAGS) $(IDT_S) -o $(OBJDIR)/idt_asm.o
	$(CC) $(CFLAGS) -c $(KERNEL) -o $(OBJDIR)/kernel.o
	$(CC) $(CFLAGS) -c $(TERMINAL) -o $(OBJDIR)/terminal.o
	$(CC) $(CFLAGS) -c $(LIBC) -o $(OBJDIR)/libc.o
	$(CC) $(CFLAGS) -c $(MEMORY) -o $(OBJDIR)/memory.o
	$(CC) $(CFLAGS) -c $(IO) -o $(OBJDIR)/io.o
	$(CC) $(CFLAGS) -c $(PORT_MANAGER) -o $(OBJDIR)/port_manager.o
	$(CC) $(CFLAGS) -c $(RTC) -o $(OBJDIR)/rtc.o
	$(CC) $(CFLAGS) -c $(GDT_C) -o $(OBJDIR)/gdt_c.o
	$(CC) $(CFLAGS) -c $(IDT_C) -o $(OBJDIR)/idt_c.o
	$(CC) $(CFLAGS) -c $(LOGGER) -o $(OBJDIR)/logger.o
	$(CC) $(CFLAGS) -c $(TEST) -o $(OBJDIR)/test.o
	$(LD) $(LDFLAGS) -o $(TARGET_KERNEL) $(OBJDIR)/boot.o $(OBJDIR)/gdt.o $(OBJDIR)/idt_asm.o $(OBJDIR)/kernel.o $(OBJDIR)/terminal.o $(OBJDIR)/libc.o $(OBJDIR)/memory.o $(OBJDIR)/io.o $(OBJDIR)/port_manager.o $(OBJDIR)/rtc.o $(OBJDIR)/gdt_c.o $(OBJDIR)/idt_c.o $(OBJDIR)/logger.o $(OBJDIR)/test.o
	mkdir -p isodir/boot/grub
	cp $(TARGET_KERNEL) isodir/boot/kernel
	cp grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o shogun-os.iso isodir
	rm -rf isodir

clean:
	rm -rf $(OBJDIR) $(BINDIR) src/idt.s src/idt_init.inc src/interrupt_handlers.inc

debug: $(OBJDIR)/kernel.o
	objdump -d $(OBJDIR)/kernel.o

info:
	@echo "CC=$(CC)"
	@echo "AS=$(AS)"
	@echo "LD=$(LD)"