# Shogun-OS

A hobby operating system kernel written in C, inspired by [sphaerophoria's Writing an Operating System](https://www.youtube.com/playlist?list=PL980gcR1LE3LBuWuSv2CL28HsfnpC4Qf7) series.

## Overview

Shogun-OS is a learning-focused operating system kernel that demonstrates fundamental concepts of low-level system programming and kernel development.

## Features

✅ Multiboot-compliant kernel  
✅ VGA text mode output with proper scrolling and new-line support  
✅ Multiboot information parsing (memory map, bootloader name)  
✅ Custom linked-list memory allocator for dynamic memory management  
✅ Cross-platform build system  
✅ UART/serial output for enhanced debugging and testing  
✅ Automated test infrastructure with pass/fail reporting  
✅ Dual output support (VGA and serial)  
✅ Port Manager system for safe I/O port allocation  
✅ Real-Time Clock (RTC) driver with CMOS access  
✅ NMI (Non-Maskable Interrupt) handling for safe hardware access  
✅ Binary format configuration for proper time encoding  

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

For serial output, you can also run:

```bash
qemu-system-i386 -cdrom shogun-os.iso -serial stdio
```

## Inspiration and Acknowledgement

This project was inspired by [sphaerophoria's Writing an Operating System](https://www.youtube.com/playlist?list=PL980gcR1LE3LBuWuSv2CL28HsfnpC4Qf7) series, who built [stream-os](https://github.com/sphaerophoria/stream-os) on Rust.

## License

This project is open source and available under the MIT License.
