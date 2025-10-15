#!/bin/bash

set -e 

echo "Assembling boot.s..."
as --32 src/boot.s -o boot.o

echo "Compiling kernel.c..."
gcc -m32 -ffreestanding -fno-stack-protector -fno-builtin -nostdlib -nostartfiles -c src/kernel.c -o kernel.o

echo "Linking kernel..."
ld -m elf_i386 -T linker.ld -o kernel boot.o kernel.o

echo "Creating ISO..."
rm -fr isodir
mkdir -p isodir/boot/grub
cp kernel isodir/boot/kernel
cp grub.cfg isodir/boot/grub/grub.cfg
grub-mkrescue -o shogun-os.iso isodir

echo "Cleaning up..."
rm -f boot.o kernel.o
rm -fr isodir

echo "Build completed successfully! Created shogun-os.iso"