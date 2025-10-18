#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#define HEAP_SIZE 2 * 1024 * 1024

extern uint32_t kernel_start;
extern uint32_t kernel_end;

typedef struct FreeSegment {
    size_t size;
    struct FreeSegment* next_segment;
} FreeSegment;

typedef struct UsedSegment {
    size_t size;
    char padding[offsetof(struct FreeSegment, next_segment) - sizeof(size_t)];
} UsedSegment;

void init_allocator(uint32_t multiboot_info_ptr);

void* allocate(size_t size, size_t alignment);

void deallocate(void* ptr);

void* malloc(size_t size);
void free(void* ptr);

void debug_print_free_list();

uint32_t get_esp();

#endif