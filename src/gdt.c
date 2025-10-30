#include "gdt.h"
#include "terminal.h"
#include "io.h"
#include <stdint.h>

static GDTEntry gdt[3];
static GDTPointer gdt_ptr;

AccessByteBuilder create_access_byte(uint8_t is_executable, uint8_t is_readable_writable, uint8_t dpl) {
    AccessByteBuilder builder;

    builder.flags = GDT_PRESENT;
    builder.flags |= (dpl << 5) & GDT_RING_3;
    builder.flags |= GDT_SEGMENT;

    if (is_executable) {
        builder.flags |= GDT_EXECUTABLE;
    }

    if (is_readable_writable) {
        builder.flags |= GDT_READABLE;
    }
    
    return builder;
}

uint8_t access_byte_build(AccessByteBuilder* builder) {
    return builder->flags;
}

void create_descriptor(GDTEntry* entry, uint32_t base, uint32_t limit, uint8_t access_byte, uint8_t flags) {
    entry->base_low = (uint16_t)(base & 0xFFFF);
    entry->base_mid = (uint8_t)((base >> 16) & 0xFF);
    entry->base_high = (uint8_t)((base >> 24) & 0xFF);
    
    // Set limit (first 16 bits)
    entry->limit_low = (uint16_t)(limit & 0xFFFF);
    
    entry->limit_high_flags = (uint8_t)((limit >> 16) & 0x0F);
    entry->limit_high_flags |= (flags & 0xF0);  
    
    entry->access_byte = access_byte;
}

// Assembly function to load GDT - declared here, defined in gdt.s
extern void load_gdt_asm(GDTPointer* gdt_ptr);

void gdt_init(void) {
    // Create null descriptor 
    gdt[0] = (GDTEntry){0};
    
    AccessByteBuilder code_builder = create_access_byte(GDT_CODE_SEGMENT, 1, 0); 
    uint8_t code_access_byte = access_byte_build(&code_builder);
    create_descriptor(&gdt[1], 0, 0xFFFFF, code_access_byte, 0xCF); // 4KB granularity, 32-bit mode

    AccessByteBuilder data_builder = create_access_byte(GDT_DATA_SEGMENT, 1, 0); 
    uint8_t data_access_byte = access_byte_build(&data_builder);
    create_descriptor(&gdt[2], 0, 0xFFFFF, data_access_byte, 0xCF); // 4KB granularity, 32-bit mode

    gdt_ptr.limit = sizeof(gdt) - 1; 
    gdt_ptr.base = (uint32_t)&gdt;   

    load_gdt_asm(&gdt_ptr);
}

void print_gdt_info(void) {
    GDTPointer current_gdt_ptr;

    __asm__ volatile (
        "sgdt %0"
        : "=m" (current_gdt_ptr)
    );
    
    output_string("GDT Info:\n");
    output_string("  Base: 0x");
    put_hex((uint32_t)current_gdt_ptr.base);
    output_string("\n");
    output_string("  Limit: 0x");
    put_hex((uint32_t)current_gdt_ptr.limit);
    output_string("\n");

    output_string("  Null Descriptor: at 0x");
    put_hex((uint32_t)&gdt[0]);
    output_string("\n");
    output_string("  Code Descriptor: at 0x");
    put_hex((uint32_t)&gdt[1]);
    output_string("\n");
    output_string("  Data Descriptor: at 0x");
    put_hex((uint32_t)&gdt[2]);
    output_string("\n");
}