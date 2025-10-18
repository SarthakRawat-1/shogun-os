#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>
#include <stddef.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_BUFFER 0xB8000
#define VGA_COLOR_WHITE_ON_BLACK 0x0F

void put_char(char c, uint8_t color, size_t x, size_t y);
void scroll_terminal();
void clear_terminal();
void terminal_put_cursor();
void terminal_put_char(char c);
void write_string(const char* str);

void int_to_string(int32_t num, char* buffer);
void put_i32(int32_t num);
void uint_to_string(uint32_t num, char* buffer);
void put_u32(uint32_t num);
void put_u64(uint64_t num);
void put_hex(uint32_t num);

void init_output();

void output_char(char c);
void output_string(const char* str);

#endif