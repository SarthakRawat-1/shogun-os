#ifndef IDT_H
#define IDT_H

#include <stdint.h>

typedef struct {
    uint16_t offset_low;      
    uint16_t selector;        
    uint8_t zero;            
    uint8_t type_attr;       
    uint16_t offset_high;    
} __attribute__((packed)) IDTEntry;

typedef struct {
    uint16_t limit;          
    uint32_t base;           
} __attribute__((packed)) IDTPointer;

#define IDT_PRESENT     0x80    
#define IDT_RING_0      0x00   
#define IDT_RING_3      0x60    
#define IDT_INTERRUPT   0x0E    
#define IDT_TRAP        0x0F    

typedef void (*interrupt_handler_t)(void);

void create_idt_descriptor(IDTEntry* entry, uint32_t offset, uint16_t selector, uint8_t type_attr);
void idt_init(void);
void idt_set_handler(uint8_t vector, uint32_t handler_offset);
void load_idt(IDTPointer* idt_ptr);

void pic_init_ports(void);
void pic_init(void);
void pic_remap(void);
void pic_send_eoi(uint8_t int_num);
void pic_unmask_irq(uint8_t irq);

void handle_divide_by_zero(void);
void handle_general_protection_fault(void);
void handle_page_fault(void);

void print_idt_info(void);

void init_interrupt_registry(void);
int register_interrupt_handler(uint8_t vector, interrupt_handler_t handler);
int unregister_interrupt_handler(uint8_t vector);

interrupt_handler_t get_interrupt_handler(uint8_t vector);

void generic_interrupt_handler_no_error_code(uint8_t vector);
void generic_interrupt_handler_error_code(uint8_t vector);

typedef enum {
    IRQ_INTERNAL,
    IRQ_PIC1,
    IRQ_PIC2,
} IrqIdType;

typedef struct {
    IrqIdType type;
    uint8_t index;  
} IrqId;

uint8_t irq_id_to_vector(IrqId irq_id);

int register_interrupt_handler_irq(IrqId irq_id, interrupt_handler_t handler);
int unregister_interrupt_handler_irq(IrqId irq_id);

#endif