#include "libc.h"
#include "terminal.h"  

void* memset(void* ptr, int value, size_t num) {
    unsigned char* p = (unsigned char*)ptr;
    for (size_t i = 0; i < num; i++) {
        p[i] = (unsigned char)value;
    }
    return ptr;
}

void* memcpy(void* dest, const void* src, size_t num) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    for (size_t i = 0; i < num; i++) {
        d[i] = s[i];
    }
    return dest;
}

void panic(const char* message) {
    clear_terminal();
    write_string("KERNEL PANIC: ");
    write_string(message);
    write_string("\nSystem halted.");

    __asm__ volatile("cli; hlt");

    while(1) {
        __asm__ volatile("hlt");
    }
}