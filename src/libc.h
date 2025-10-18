#ifndef LIBC_H
#define LIBC_H

#include <stdint.h>
#include <stddef.h>

void* memset(void* ptr, int value, size_t num);
void* memcpy(void* dest, const void* src, size_t num);
void panic(const char* message);

#endif