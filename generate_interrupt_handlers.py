#!/usr/bin/env python3

def generate_asm_handler(num):
    return f"""
.globl interrupt_handler_{num}
interrupt_handler_{num}:
    pushal              # Save all general purpose registers
    pushl %ds           # Save segment registers
    pushl %es
    pushl %fs
    pushl %gs
    
    # Set up data segments (kernel segments)
    movl $0x10, %eax    # Data segment selector
    movl %eax, %ds
    movl %eax, %es
    movl %eax, %fs
    movl %eax, %gs
    
    # Push interrupt vector number ({num}) to stack as argument
    pushl ${num}
    call generic_interrupt_handler_no_error_code
    addl $4, %esp       # Clean up stack (remove the interrupt number)
    
    # Restore segment registers
    popl %gs
    popl %fs
    popl %es
    popl %ds
    
    popal               # Restore all general purpose registers
    iret                # Return from interrupt
"""

def generate_error_code_handler(num):
    if num == 0: 
        return f"""
.globl interrupt_handler_{num}
interrupt_handler_{num}:
    pushal
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs
    
    movl $0x10, %eax
    movl %eax, %ds
    movl %eax, %es
    movl %eax, %fs
    movl %eax, %gs
    
    call handle_divide_by_zero
    
    popl %gs
    popl %fs
    popl %es
    popl %ds
    popal
    iret
"""
    elif num == 13: 
        return f"""
.globl interrupt_handler_{num}
interrupt_handler_{num}:
    pushal
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs
    
    movl $0x10, %eax
    movl %eax, %ds
    movl %eax, %es
    movl %eax, %fs
    movl %eax, %gs
    
    call handle_general_protection_fault
    
    popl %gs
    popl %fs
    popl %es
    popl %ds
    popal
    iret
"""
    elif num == 14:  
        return f"""
.globl interrupt_handler_{num}
interrupt_handler_{num}:
    pushal
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs
    
    movl $0x10, %eax
    movl %eax, %ds
    movl %eax, %es
    movl %eax, %fs
    movl %eax, %gs
    
    call handle_page_fault
    
    popl %gs
    popl %fs
    popl %es
    popl %ds
    popal
    iret
"""
    elif num in [8, 10, 11, 12, 14, 17]:  
        return f"""
.globl interrupt_handler_{num}
interrupt_handler_{num}:
    pushal              # Save all general purpose registers
    pushl %ds           # Save segment registers
    pushl %es
    pushl %fs
    pushl %gs
    
    # Set up data segments (kernel segments)
    movl $0x10, %eax    # Data segment selector
    movl %eax, %ds
    movl %eax, %es
    movl %eax, %fs
    movl %eax, %gs
    
    # Push interrupt vector number ({num}) to stack as argument
    pushl ${num}
    call generic_interrupt_handler_error_code
    addl $4, %esp       # Clean up stack (remove the interrupt number)
    
    # Restore segment registers
    popl %gs
    popl %fs
    popl %es
    popl %ds
    
    popal               # Restore all general purpose registers
    iret                # Return from interrupt
"""
    else:
        return generate_asm_handler(num)

def main():
    with open("src/idt.s", "w") as f:
        f.write("# Generated interrupt handlers\n")
        f.write(".section .text\n.code32\n\n")

        f.write("""# Assembly functions for IDT

# Function to load IDT using LIDT instruction
.globl load_idt_asm
.extern generic_interrupt_handler_no_error_code
.extern generic_interrupt_handler_error_code
.extern handle_divide_by_zero
.extern handle_general_protection_fault
.extern handle_page_fault

load_idt_asm:
    pushl   %ebp
    movl    %esp, %ebp
    movl    8(%ebp), %eax       # Get pointer to IDT pointer structure
    lidt    (%eax)              # Load IDT
    movl    %ebp, %esp
    popl    %ebp
    ret

""")

        for i in range(256):
            f.write(generate_error_code_handler(i))
        
        f.write("\n.section .note.GNU-stack,\"\",@progbits\n")

if __name__ == "__main__":
    main()