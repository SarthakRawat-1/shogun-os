.section .text
.code32

.globl load_gdt_asm
.extern kernel_main

load_gdt_asm:
    pushl   %ebp
    movl    %esp, %ebp
    movl    8(%ebp), %eax       
    lgdt    (%eax)              # Load GDT
    
    movw    $0x10, %ax         
    movw    %ax, %ds            
    movw    %ax, %es            
    movw    %ax, %fs            
    movw    %ax, %gs            
    movw    %ax, %ss            

    ljmpl   $0x08, $reload_cs   

reload_cs:
    movl    %ebp, %esp
    popl    %ebp
    ret

.section .note.GNU-stack,"",@progbits