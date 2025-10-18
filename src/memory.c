#include "memory.h"
#include "terminal.h"
#include "io.h"
#include <stddef.h>
#include <stdbool.h>

struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t num;
    uint32_t size;
    uint32_t addr;
    uint32_t shndx;
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_ctrl_info;
    uint32_t vbe_mode_info;
    uint32_t vbe_mode;
    uint32_t vbe_interface_seg;
    uint32_t vbe_interface_off;
    uint32_t vbe_interface_len;
};

struct memory_map_entry {
    uint32_t size;  
    uint32_t base_addr_low;
    uint32_t base_addr_high;
    uint32_t length_low;
    uint32_t length_high;
    uint32_t type;
};

static inline FreeSegment* atomic_load_FreeSegment_ptr(FreeSegment* volatile* ptr) {
    FreeSegment* result;
    __asm__ volatile ("movl %1, %0" : "=r" (result) : "m" (*ptr) : "memory");
    return result;
}

static inline void atomic_store_FreeSegment_ptr(FreeSegment* volatile* ptr, FreeSegment* val) {
    __asm__ volatile ("movl %1, %0" : "=m" (*ptr) : "r" (val) : "memory");
}

static volatile FreeSegment* first_free = NULL;

static uint8_t heap_area[HEAP_SIZE];

uint32_t get_esp() {
    uint32_t esp;
    __asm__ volatile ("movl %%esp, %0" : "=r" (esp));
    return esp;
}

static inline size_t align_up(size_t value, size_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

void init_allocator(uint32_t multiboot_info_ptr) {
    struct multiboot_info* mb_info = (struct multiboot_info*)multiboot_info_ptr;
    
    if (!(mb_info->flags & (1 << 6))) {
        return;
    }

    uint32_t mmap_addr = mb_info->mmap_addr;
    uint32_t mmap_length = mb_info->mmap_length;
    
    uint32_t current = mmap_addr;
    uint32_t end = mmap_addr + mmap_length;
    
    uint32_t largest_region_addr = 0;
    uint32_t largest_region_size = 0;
    
    while (current < end) {
        struct memory_map_entry* entry = (struct memory_map_entry*)current;
        
        if (entry->type == 1 && entry->length_low > largest_region_size) {
            if (entry->length_low >= HEAP_SIZE) {
                largest_region_addr = entry->base_addr_low;
                largest_region_size = entry->length_low;
            }
        }
        
        current += entry->size + 4;
    }
    
    if (largest_region_size == 0) {
        return;
    }

    uint32_t kernel_end_addr = (uint32_t)&kernel_end;
    uint32_t esp = get_esp();

    uint32_t reserved_end = kernel_end_addr > esp ? kernel_end_addr : esp;

    if (largest_region_addr <= reserved_end && 
        (largest_region_addr + largest_region_size) > reserved_end) {
        uint32_t heap_start = reserved_end;

        heap_start = (heap_start + 7) & ~7;

        FreeSegment* initial_segment = (FreeSegment*)heap_start;
        initial_segment->size = largest_region_size - (heap_start - largest_region_addr) - sizeof(FreeSegment);
        initial_segment->next_segment = NULL;

        atomic_store_FreeSegment_ptr((FreeSegment* volatile*)&first_free, initial_segment);
    }
}

void* allocate(size_t size, size_t alignment) {
    if (size == 0) {
        return NULL;
    }

    size_t total_size = sizeof(UsedSegment) + size;

    FreeSegment* prev = NULL;
    FreeSegment* current = atomic_load_FreeSegment_ptr((FreeSegment* volatile*)&first_free);
    
    while (current != NULL) {
        uint32_t segment_end = (uint32_t)current + sizeof(FreeSegment) + current->size;

        uint32_t aligned_data_ptr = (segment_end - size) & ~(alignment - 1);

        uint32_t header_ptr = aligned_data_ptr - sizeof(UsedSegment);

        uint32_t data_start = (uint32_t)current + sizeof(FreeSegment);
        
        if (header_ptr >= data_start) {
            size_t full_alloc_size = (segment_end - header_ptr);

            if (full_alloc_size <= current->size) {
                UsedSegment* used_header = (UsedSegment*)header_ptr;
                used_header->size = full_alloc_size; 

                size_t remaining_size = current->size - full_alloc_size;
                
                if (remaining_size == 0) {
                    if (prev == NULL) {
                        atomic_store_FreeSegment_ptr((FreeSegment* volatile*)&first_free, current->next_segment);
                    } else {
                        prev->next_segment = current->next_segment;
                    }
                } else if (remaining_size < sizeof(FreeSegment)) {
                    current->size = current->size - full_alloc_size;
                } else {
                    current->size = current->size - full_alloc_size;
                }
                
                return (void*)aligned_data_ptr;
            }
        }
        
        prev = current;
        current = current->next_segment;
    }
    
    return NULL;
}

static bool segments_adjacent(FreeSegment* first, FreeSegment* second) {
    uint32_t first_end = (uint32_t)first + sizeof(FreeSegment) + first->size;
    return (uint32_t)second == first_end;
}

static FreeSegment* merge_adjacent(FreeSegment* first, FreeSegment* second) {
    if (segments_adjacent(first, second)) {
        first->size = (uint32_t)second + sizeof(FreeSegment) + second->size - ((uint32_t)first + sizeof(FreeSegment));
        first->next_segment = second->next_segment;
        return first;
    }
    return first;
}

void deallocate(void* ptr) {
    if (ptr == NULL) {
        return;
    }
    
    UsedSegment* used_header = (UsedSegment*)((uint8_t*)ptr - sizeof(UsedSegment));

    size_t total_size = used_header->size;

    FreeSegment* freed_block = (FreeSegment*)used_header;
    freed_block->size = total_size - sizeof(UsedSegment);
    freed_block->next_segment = NULL;

    FreeSegment* prev = NULL;
    FreeSegment* current = atomic_load_FreeSegment_ptr((FreeSegment* volatile*)&first_free);
    
    while (current != NULL && (uint32_t)current < (uint32_t)freed_block) {
        prev = current;
        current = current->next_segment;
    }
    
    freed_block->next_segment = current;
    if (prev == NULL) {
        atomic_store_FreeSegment_ptr((FreeSegment* volatile*)&first_free, freed_block);
    } else {
        prev->next_segment = freed_block;
    }
    
    if (current != NULL && segments_adjacent(freed_block, current)) {
        freed_block->size = freed_block->size + sizeof(FreeSegment) + current->size;
        freed_block->next_segment = current->next_segment;
    }

    if (prev != NULL && segments_adjacent(prev, freed_block)) {
        prev->size = prev->size + sizeof(FreeSegment) + freed_block->size;
        prev->next_segment = freed_block->next_segment;
    }
}

void* malloc(size_t size) {
    return allocate(size, 8);
}

void free(void* ptr) {
    deallocate(ptr);
}

void debug_print_free_list() {
    FreeSegment* current = atomic_load_FreeSegment_ptr((FreeSegment* volatile*)&first_free);
    int index = 0;
    
    if (current == NULL) {
        output_string("Free list is empty\\n");
        return;
    }
    
    while (current != NULL) {
        uint32_t start_addr = (uint32_t)current;
        uint32_t end_addr = start_addr + sizeof(FreeSegment) + current->size;
        
        put_hex(start_addr);
        output_string(": ");
        put_hex(end_addr);
        output_string(", FreeSegment { size: ");
        put_u32(current->size);
        output_string(", next_segment: ");
        
        if (current->next_segment) {
            put_hex((uint32_t)current->next_segment);
        } else {
            output_string("0x0");
        }
        output_string(" }\\n");
        
        current = current->next_segment;
        index++;
    }
}