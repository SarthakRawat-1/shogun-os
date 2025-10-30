#!/usr/bin/env python3

def generate_idt_init_code():
    code = ""

    for i in range(256):
        if i == 0:
            code += f"    create_idt_descriptor(&idt[{i}], (uint32_t)interrupt_handler_{i}, 0x08, IDT_PRESENT | IDT_INTERRUPT);   // Divide by zero\n"
        elif i == 13:
            code += f"    create_idt_descriptor(&idt[{i}], (uint32_t)interrupt_handler_{i}, 0x08, IDT_PRESENT | IDT_INTERRUPT);  // General Protection Fault\n"
        elif i == 14:
            code += f"    create_idt_descriptor(&idt[{i}], (uint32_t)interrupt_handler_{i}, 0x08, IDT_PRESENT | IDT_INTERRUPT);  // Page Fault\n"
        else:
            code += f"    create_idt_descriptor(&idt[{i}], (uint32_t)interrupt_handler_{i}, 0x08, IDT_PRESENT | IDT_INTERRUPT);\n"
    
    return code

def main():
    with open("src/idt_init.inc", "w") as f:
        f.write(generate_idt_init_code())

if __name__ == "__main__":
    main()