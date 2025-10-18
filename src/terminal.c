#include "terminal.h"
#include "io.h"

static uint16_t* vga_buffer = (uint16_t*)VGA_BUFFER;
static uint8_t terminal_row = 0;
static uint8_t terminal_column = 0;
static uint8_t terminal_color = VGA_COLOR_WHITE_ON_BLACK;

void put_char(char c, uint8_t color, size_t x, size_t y) {
    uint16_t entry = (uint16_t)c | (uint16_t)color << 8;
    vga_buffer[y * VGA_WIDTH + x] = entry;
}

void scroll_terminal() {
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }

    for (size_t x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = ' ' | (uint16_t)terminal_color << 8;
    }
}

void clear_terminal() {
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            put_char(' ', terminal_color, x, y);
        }
    }
    terminal_row = 0;
    terminal_column = 0;
}

void terminal_put_cursor() {
    terminal_column++;
    if (terminal_column >= VGA_WIDTH) {
        terminal_column = 0;
        terminal_row++;
    }
    
    if (terminal_row >= VGA_HEIGHT) {
        scroll_terminal();
        terminal_row = VGA_HEIGHT - 1;
    }
}

void terminal_put_char(char c) {
    if (c == '\n') {
        terminal_column = 0;
        terminal_row++;
        
        if (terminal_row >= VGA_HEIGHT) {
            scroll_terminal();
            terminal_row = VGA_HEIGHT - 1;
        }
    } else {
        put_char(c, terminal_color, terminal_column, terminal_row);
        terminal_put_cursor();
    }
}

void write_string(const char* str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        terminal_put_char(str[i]);
    }
}

void int_to_string(int32_t num, char* buffer) {
    int i = 0;
    int negative = 0;

    if (num == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    if (num < 0) {
        negative = 1;
        num = -num;
    }

    while (num != 0) {
        buffer[i++] = (num % 10) + '0';
        num = num / 10;
    }

    if (negative) {
        buffer[i++] = '-';
    }

    buffer[i] = '\0';
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = temp;
        start++;
        end--;
    }
}

void put_i32(int32_t num) {
    char buffer[12]; 
    int_to_string(num, buffer);
    output_string(buffer);
}

void uint_to_string(uint32_t num, char* buffer) {
    int i = 0;

    if (num == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    while (num != 0) {
        buffer[i++] = (num % 10) + '0';
        num = num / 10;
    }

    buffer[i] = '\0';
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = temp;
        start++;
        end--;
    }
}

void put_u32(uint32_t num) {
    char buffer[11]; 
    uint_to_string(num, buffer);
    output_string(buffer);
}

void put_u64(uint64_t num) {
    uint32_t upper = (uint32_t)(num >> 32);
    uint32_t lower = (uint32_t)num;
    
    if (upper != 0) {
        put_u32(upper);
        char buffer[11];
        uint_to_string(lower, buffer);
        if (lower < 10) {
            output_string("000000000");
        } else if (lower < 100) {
            output_string("00000000");
        } else if (lower < 1000) {
            output_string("0000000");
        } else if (lower < 10000) {
            output_string("000000");
        } else if (lower < 100000) {
            output_string("00000");
        } else if (lower < 1000000) {
            output_string("0000");
        } else if (lower < 10000000) {
            output_string("000");
        } else if (lower < 100000000) {
            output_string("00");
        } else if (lower < 1000000000) {
            output_string("0");
        }
        output_string(buffer);
    } else {
        put_u32(lower);
    }
}

void put_hex(uint32_t num) {
    char hex_chars[] = "0123456789ABCDEF";
    char buffer[11]; 
    buffer[0] = '0';
    buffer[1] = 'x';
    
    int i;
    for (i = 8; i > 0; i--) {
        buffer[i + 1] = hex_chars[num & 0xF];
        num >>= 4;
    }
    buffer[10] = '\0';
    
    output_string(&buffer[0]);
}

void init_output() {
    init_serial();
}

void output_char(char c) {
    terminal_put_char(c);

    write_serial(c);
}

void output_string(const char* str) {
    write_string(str);
    
    write_serial_string(str);
}