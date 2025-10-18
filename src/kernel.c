#include <stdint.h>
#include <stddef.h>
#include "terminal.h"
#include "libc.h"
#include "multiboot.h"
#include "memory.h"

void kernel_main(uint32_t magic, uint32_t multiboot_info_ptr) {
    clear_terminal();
    
    write_string("hi shogun from c\n");

    write_string("Multiboot magic: ");
    put_hex(magic);
    write_string("\n");
    
    write_string("Multiboot info ptr: ");
    put_hex(multiboot_info_ptr);
    write_string("\n");

    struct multiboot_info* mb_info = (struct multiboot_info*)multiboot_info_ptr;
    
    if (magic == 0x2BADB002) {
        write_string("Multiboot info valid!\n");

        if (mb_info->flags & (1 << 9)) {  
            write_string("Bootloader name addr: ");
            put_hex(mb_info->boot_loader_name);
            write_string("\n");
            
            if (mb_info->boot_loader_name != 0) {
                write_string("Bootloader name: ");
                char* bootloader_name = (char*)mb_info->boot_loader_name;
                write_string(bootloader_name);
                write_string("\n");

                write_string("Bootloader chars: ");
                int i = 0;
                while (bootloader_name[i] != '\0' && i < 50) { 
                    terminal_put_char(bootloader_name[i]);
                    if (bootloader_name[i+1] != '\0') {
                        terminal_put_char(' ');
                    }
                    i++;
                }
                write_string("\n");
            }
        } else {
            write_string("Bootloader name flag not set\n");
        }

        if (mb_info->flags & (1 << 6)) { 
            write_string("Memory map entries:\n");

            uint32_t mmap_addr = mb_info->mmap_addr;
            uint32_t mmap_length = mb_info->mmap_length;
            
            uint32_t current = mmap_addr;
            uint32_t end = mmap_addr + mmap_length;
            
            while (current < end) {
                struct memory_map_entry* entry = (struct memory_map_entry*)current;
                
                write_string("Addr: ");
                put_hex(entry->base_addr_low);
                write_string(" Size: ");
                put_hex(entry->length_low);
                write_string(" Type: ");
                put_u32(entry->type);
                write_string("\n");
                

                current += entry->size + 4;
            }
        }
    } else {
        write_string("Invalid multiboot magic!\n");
    }
    
    write_string("Testing integer printing:\n");
    put_i32(-42);
    write_string("\n");
    put_u32(12345);
    write_string("\n");
    put_u64(0x123456789ABCDEF0);
    write_string("\n");

    write_string("Initializing memory allocator...\n");
    init_allocator(multiboot_info_ptr);
    
    write_string("Testing memory allocator:\n");

    write_string("Kernel start: ");
    put_hex((uint32_t)&kernel_start);
    write_string(", Kernel end: ");
    put_hex((uint32_t)&kernel_end);
    write_string("\n");

    write_string("Pre-dealloc:\n");
    debug_print_free_list();

    char* test_str = (char*)malloc(20);
    if (test_str != NULL) {
        write_string("Allocation successful!\n");

        const char* msg = "Hello from heap!";
        for (int i = 0; msg[i] != '\0' && i < 20; i++) {
            test_str[i] = msg[i];
        }
        test_str[15] = '\0';
        
        write_string("Allocated string: ");
        write_string(test_str);
        write_string("\n");

        write_string("After first allocation:\n");
        debug_print_free_list();

        int* numbers = (int*)malloc(5 * sizeof(int));
        if (numbers != NULL) {
            write_string("Multiple allocation test successful!\n");

            for (int i = 0; i < 5; i++) {
                numbers[i] = i * 10;
            }

            for (int i = 0; i < 5; i++) {
                put_i32(numbers[i]);
                terminal_put_char(' ');
            }
            write_string("\n");

            write_string("After second allocation:\n");
            debug_print_free_list();

            free(numbers);
            write_string("Freed the integer array.\n");
            
            write_string("After first deallocation:\n");
            debug_print_free_list();
        }

        free(test_str);
        write_string("Freed the string allocation.\n");
        
        write_string("Post-dealloc:\n");
        debug_print_free_list();
    } else {
        write_string("Allocation failed!\n");
    }
    
    write_string("\nAvailable memory segments...\n");
    struct multiboot_info* mb_info_local = (struct multiboot_info*)multiboot_info_ptr;
    if (mb_info_local->flags & (1 << 6)) {
        uint32_t mmap_addr = mb_info_local->mmap_addr;
        uint32_t mmap_length = mb_info_local->mmap_length;
        
        uint32_t current = mmap_addr;
        uint32_t end = mmap_addr + mmap_length;
        int count = 0;
        
        uint32_t temp = mmap_addr;
        while (temp < end) {
            struct memory_map_entry* entry = (struct memory_map_entry*)temp;
            count++;
            temp += entry->size + 4;
        }
        
        write_string("num_mmap_entries: ");
        put_i32(count);
        write_string("\n");

        current = mmap_addr;
        while (current < end) {
            struct memory_map_entry* entry = (struct memory_map_entry*)current;
            write_string("size: ");
            put_u32(entry->size);
            write_string(", len: ");

            uint32_t len_low = entry->length_low;
            if (len_low >= (1024 * 1024)) {  
                put_u32(len_low / (1024 * 1024));
                write_string("M");
            } else if (len_low >= 1024) {  
                put_u32(len_low / 1024);
                write_string("K");
            } else {
                put_u32(len_low);
                write_string("B");
            }
            
            write_string(", addr: ");
            put_hex(entry->base_addr_low);
            write_string("\n");
            
            current += entry->size + 4;
        }
    }
    
    write_string("Memory allocator test completed.\n");
}