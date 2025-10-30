#include "idt.h"
#include "io.h"
#include "terminal.h"
#include "port_manager.h"
#include "logger.h"
#include <stdint.h>

static IDTEntry idt[256];
static IDTPointer idt_ptr;

static interrupt_handler_t interrupt_handlers[256];
static uint8_t handlers_initialized = 0; 

void create_idt_descriptor(IDTEntry* entry, uint32_t offset, uint16_t selector, uint8_t type_attr) {
    entry->offset_low = (uint16_t)(offset & 0xFFFF);
    entry->offset_high = (uint16_t)((offset >> 16) & 0xFFFF);
    entry->selector = selector;
    entry->zero = 0;
    entry->type_attr = type_attr;
}

void idt_set_handler(uint8_t vector, uint32_t handler_offset) {
    create_idt_descriptor(&idt[vector], handler_offset, 0x08, IDT_PRESENT | IDT_INTERRUPT);
}

extern void load_idt_asm(IDTPointer* idt_ptr);

#include "interrupt_handlers.inc"

extern void handle_divide_by_zero(void);
extern void handle_general_protection_fault(void);
extern void handle_page_fault(void);

void init_interrupt_registry(void) {
    if (!handlers_initialized) {
        for (int i = 0; i < 256; i++) {
            interrupt_handlers[i] = NULL;
        }
        handlers_initialized = 1;
    }
}

int register_interrupt_handler(uint8_t vector, interrupt_handler_t handler) {
    if (!handlers_initialized) {
        init_interrupt_registry();
    }
    
    if (vector >= 256) {
        return -1;  
    }
    
    interrupt_handlers[vector] = handler;
    return 0;  
}

int unregister_interrupt_handler(uint8_t vector) {
    if (!handlers_initialized) {
        init_interrupt_registry();
    }
    
    if (vector >= 256) {
        return -1;  
    }
    
    interrupt_handlers[vector] = NULL;
    return 0;  
}

interrupt_handler_t get_interrupt_handler(uint8_t vector) {
    if (!handlers_initialized) {
        init_interrupt_registry();
    }
    
    if (vector >= 256) {
        return NULL;  
    }
    
    return interrupt_handlers[vector];
}

void idt_init(void) {
    pic_init_ports();

    for (int i = 0; i < 256; i++) {
        idt[i].offset_low = 0;
        idt[i].offset_high = 0;
        idt[i].selector = 0;
        idt[i].zero = 0;
        idt[i].type_attr = 0;
    }

    #include "idt_init.inc"

    idt_ptr.limit = sizeof(idt) - 1; 
    idt_ptr.base = (uint32_t)&idt;   
    
    load_idt_asm(&idt_ptr);
    
    output_string("IDT initialized and loaded successfully!\n");
}

void generic_interrupt_handler_no_error_code(uint8_t vector) {
    if (handlers_initialized && vector < 256 && interrupt_handlers[vector] != NULL) {
        interrupt_handlers[vector]();
    }

    pic_send_eoi(vector);
}

void generic_interrupt_handler_error_code(uint8_t vector) {
    // The stack layout is: [error_code, eip, cs, eflags]
    uint32_t error_code;
    __asm__ volatile ("add $4, %%esp" : "=a" (error_code)); 
    
    if (handlers_initialized && vector < 256 && interrupt_handlers[vector] != NULL) {
        interrupt_handlers[vector]();
    }

    pic_send_eoi(vector);
}

void handle_divide_by_zero(void) {
    output_string("Divide by zero exception occurred!\n");

    output_string("System halted due to divide by zero.\n");
    __asm__ volatile ("cli; hlt");
}

void handle_general_protection_fault(void) {
    output_string("General Protection Fault occurred!\n");
    output_string("System halted due to GPF.\n");
    __asm__ volatile ("cli; hlt");
}

void handle_page_fault(void) {
    output_string("Page Fault occurred!\n");
    output_string("System halted due to page fault.\n");
    __asm__ volatile ("cli; hlt");
}

void print_idt_info(void) {
    output_string("IDT Info:\n");
    output_string("  Base: 0x");
    put_hex((uint32_t)idt_ptr.base);
    output_string("\n");
    output_string("  Limit: 0x");
    put_hex((uint32_t)idt_ptr.limit);
    output_string("\n");
    output_string("  Number of entries: 256\n");

    if (idt[0x80].offset_low != 0 || idt[0x80].offset_high != 0) {
        output_string("  Interrupt 0x80 handler: SET\n");
    } else {
        output_string("  Interrupt 0x80 handler: NOT SET\n");
    }
    
    if (idt[0].offset_low != 0 || idt[0].offset_high != 0) {
        output_string("  Divide by zero handler: SET\n");
    } else {
        output_string("  Divide by zero handler: NOT SET\n");
    }
    
    if (idt[13].offset_low != 0 || idt[13].offset_high != 0) {
        output_string("  General Protection Fault handler: SET\n");
    } else {
        output_string("  General Protection Fault handler: NOT SET\n");
    }
}

static PortHandle* master_pic_cmd = NULL;
static PortHandle* master_pic_data = NULL;
static PortHandle* slave_pic_cmd = NULL;
static PortHandle* slave_pic_data = NULL;

void pic_init_ports(void) {
    master_pic_cmd = request_port(0x20);
    master_pic_data = request_port(0x21);
    slave_pic_cmd = request_port(0xA0);
    slave_pic_data = request_port(0xA1);
}

void pic_send_eoi(uint8_t int_num) {
    if(int_num >= 0x28 && int_num <= 0x2F) {
        if (slave_pic_cmd) {
            write_port_b(slave_pic_cmd, 0x20);
        }
        if (master_pic_cmd) {
            write_port_b(master_pic_cmd, 0x20);
        }
    } else if(int_num >= 0x20 && int_num <= 0x27) {
        if (master_pic_cmd) {
            write_port_b(master_pic_cmd, 0x20);
        }
    }
}

void pic_init(void) {
    if (master_pic_data && slave_pic_data) {
        write_port_b(master_pic_data, 0xFF);  
        write_port_b(slave_pic_data, 0xFF);   
        
        output_string("PIC interrupts masked successfully!\n");
    } else {
        output_string("PIC initialization failed - couldn't acquire ports!\n");
    }
}

// Function to remap PIC (from default 0x20-0x27 and 0x28-0x2F to 0x40-0x47 and 0x48-0x4F)
// This avoids conflicts with CPU exceptions (0-31)
void pic_remap(void) {
    if (master_pic_cmd && slave_pic_cmd && master_pic_data && slave_pic_data) {
        output_string("PIC remapping: Before - Master vectors 0x20-0x27, Slave vectors 0x28-0x2F\n");

        uint8_t master_mask = read_port_b(master_pic_data);
        uint8_t slave_mask = read_port_b(slave_pic_data);

        write_port_b(master_pic_cmd, 0x11);  
        write_port_b(slave_pic_cmd, 0x11);  
        
        write_port_b(master_pic_data, 0x40); 
        write_port_b(slave_pic_data, 0x48); 

        write_port_b(master_pic_data, 0x04);
        write_port_b(slave_pic_data, 0x02);  

        write_port_b(master_pic_data, 0x01);
        write_port_b(slave_pic_data, 0x01);  

        write_port_b(master_pic_data, master_mask);
        write_port_b(slave_pic_data, slave_mask);
        
        output_string("PIC remapped successfully! After - Master vectors 0x40-0x47, Slave vectors 0x48-0x4F\n");
    } else {
        output_string("PIC remapping failed - couldn't acquire ports!\n");
    }
}

uint8_t irq_id_to_vector(IrqId irq_id) {
    switch (irq_id.type) {
        case IRQ_INTERNAL:
            return irq_id.index;
        case IRQ_PIC1:
            return 64 + irq_id.index;  // PIC1 offset is 64 after remapping
        case IRQ_PIC2:
            return 72 + irq_id.index;  // PIC2 offset is 72 after remapping
        default:
            return 0; 
    }
}

int register_interrupt_handler_irq(IrqId irq_id, interrupt_handler_t handler) {
    uint8_t vector = irq_id_to_vector(irq_id);
    return register_interrupt_handler(vector, handler);
}

int unregister_interrupt_handler_irq(IrqId irq_id) {
    uint8_t vector = irq_id_to_vector(irq_id);
    return unregister_interrupt_handler(vector);
}