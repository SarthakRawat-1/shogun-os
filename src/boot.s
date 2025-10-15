.set MULTIBOOT_PAGE_ALIGN, 1<<0
.set MULTIBOOT_MEMORY_INFO, 1<<1
.set MULTIBOOT_HEADER_MAGIC, 0x1BADB002
.set MULTIBOOT_HEADER_FLAGS, MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO
.set MULTIBOOT_CHECKSUM, -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

.section .multiboot
.align 4
.long MULTIBOOT_HEADER_MAGIC
.long MULTIBOOT_HEADER_FLAGS
.long MULTIBOOT_CHECKSUM

# 16KiB for stack
.section .bss
.align 16
stack_bottom:
.skip 16384
stack_top:

# Entry point
.section .text
.global _start
_start:
    mov $stack_top, %esp

    # Parameters in reverse order because C convention 
    push %ebx
    push %eax

    call kernel_main

    add $8, %esp  # Remove 8 bytes (2 parameters * 4 bytes each)

    cli
1:  hlt
    jmp 1b

.section .note.GNU-stack,"",@progbits
