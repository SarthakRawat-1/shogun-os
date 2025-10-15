# Shogun-OS

A hobby operating system kernel written in C, inspired by [sphaerophoria's Writing an Operating System](https://www.youtube.com/playlist?list=PL980gcR1LE3LBuWuSv2CL28HsfnpC4Qf7) series.

## Overview

Shogun-OS is a learning-focused operating system kernel that demonstrates fundamental concepts of low-level system programming and kernel development. The project implements a kernel that boots in QEMU, parses multiboot information, displays output using VGA text mode, and provides C-based debugging utilities and integer printing functions.

## Features

- Multiboot-compliant kernel
- VGA text mode output with proper scrolling and new-line support
- Assembly bootloader with proper stack initialization and multiboot parameter passing
- C kernel implementation with integer printing (signed, unsigned 32/64-bit, hex)
- Multiboot information parsing (memory map, bootloader name)
- Cross-platform build system

## Installation

### Prerequisites

To build Shogun-OS, you'll need the following tools installed:

**On Ubuntu/Debian (including WSL):**

```bash
sudo apt update
sudo apt install build-essential nasm gcc-multilib xorriso grub-pc-bin mtools qemu-system-x86
```

**On Arch Linux:**

```bash
sudo pacman -S base-devel nasm xorriso grub mtools qemu
```

**On macOS:**

```bash
# Install Homebrew if you haven't already
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install required tools
brew install gcc nasm xorriso grub mtools qemu
```

## Building

Clone or navigate to the Shogun-OS directory:

```bash
cd shogun-os
```

Use the Makefile:

```bash
make
```

This will generate a `shogun-os.iso` file that can be booted in QEMU.

## Running

To run the kernel in QEMU:

```bash
qemu-system-i386 -cdrom shogun-os.iso
```

When run, the kernel will:

- Display "hi shogun from c"
- Show multiboot information (magic value, info pointer)
- Print the bootloader name (e.g., "GRUB 2.12-1ubuntu7.3")
- Show memory map entries with address, size, and type
- Test integer printing functions

## Project Structure

- `src/boot.s` - Assembly bootloader with multiboot header and parameter passing
- `src/kernel.c` - Main kernel implementation with VGA output, integer printing, and multiboot parsing
- `linker.ld` - Linker script for memory layout
- `grub.cfg` - GRUB bootloader configuration
- `build.sh` - Build script
- `Makefile` - Alternative build system
- `DEVELOPMENT_LOG.txt` - Log of implemented features

## Implementation Details

The kernel implements functionality as described in OS development tutorials:

- Proper assembly-to-C calling convention with multiboot parameter passing (EAX, EBX)
- Multiboot information structure parsing to retrieve memory map and bootloader name
- Integer printing functions (put_i32, put_u32, put_u64, put_hex)
- New-line handling with automatic scrolling
- Basic C library functions (memset, memcpy, panic handler)

## Inspiration and Acknowledgement

This project was inspired by [sphaerophoria's Writing an Operating System](https://www.youtube.com/playlist?list=PL980gcR1LE3LBuWuSv2CL28HsfnpC4Qf7) series, who built [stream-os](https://github.com/sphaerophoria/stream-os) on Rust.

## License

This project is open source and available under the MIT License.
