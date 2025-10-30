#ifndef GDT_H
#define GDT_H

#include <stdint.h>

// Global Descriptor Table entry structure 
typedef struct {
    uint16_t limit_low;      
    uint16_t base_low;       // Base address bits 0-15
    uint8_t base_mid;        // Base address bits 16-23
    uint8_t access_byte;     
    uint8_t limit_high_flags; 
    uint8_t base_high;       
} __attribute__((packed)) GDTEntry;

typedef struct {
    uint16_t limit;          
    uint32_t base;           // Base address of GDT
} __attribute__((packed)) GDTPointer;

// Access byte flags
#define GDT_PRESENT       0x80    
#define GDT_RING_0        0x00    
#define GDT_RING_1        0x20    
#define GDT_RING_2        0x40   
#define GDT_RING_3        0x60    
#define GDT_SEGMENT       0x10    
#define GDT_EXECUTABLE    0x08    
#define GDT_CONFORMING    0x04    
#define GDT_READABLE      0x02    
#define GDT_WRITABLE      0x02    
#define GDT_ACCESSED      0x01    

#define GDT_CODE_SEGMENT  1
#define GDT_DATA_SEGMENT  0

typedef struct {
    uint8_t flags;
} AccessByteBuilder;

AccessByteBuilder create_access_byte(uint8_t is_executable, uint8_t is_readable_writable, uint8_t dpl);

uint8_t access_byte_build(AccessByteBuilder* builder);

void create_descriptor(GDTEntry* entry, uint32_t base, uint32_t limit, uint8_t access_byte, uint8_t flags);

void gdt_init(void);

void print_gdt_info(void);

#endif