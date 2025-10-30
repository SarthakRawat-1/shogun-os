#!/usr/bin/env python3

def generate_extern_declarations():
    code = ""
    
    for i in range(256):
        code += f"extern void interrupt_handler_{i}(void);\n"
    
    return code

def main():
    with open("src/interrupt_handlers.inc", "w") as f:
        f.write(generate_extern_declarations())

if __name__ == "__main__":
    main()