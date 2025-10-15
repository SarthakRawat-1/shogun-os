#include <stdint.h>
#include <stddef.h>

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
void* memset(void* ptr, int value, size_t num);
void* memcpy(void* dest, const void* src, size_t num);
void panic(const char* message);

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

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_BUFFER 0xB8000
#define VGA_COLOR_WHITE_ON_BLACK 0x0F

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
    write_string(buffer);
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
    write_string(buffer);
}

void put_u64(uint64_t num) {
    uint32_t upper = (uint32_t)(num >> 32);
    uint32_t lower = (uint32_t)num;
    
    if (upper != 0) {
        put_u32(upper);
        char buffer[11];
        uint_to_string(lower, buffer);
        if (lower < 10) {
            write_string("000000000");
        } else if (lower < 100) {
            write_string("00000000");
        } else if (lower < 1000) {
            write_string("0000000");
        } else if (lower < 10000) {
            write_string("000000");
        } else if (lower < 100000) {
            write_string("00000");
        } else if (lower < 1000000) {
            write_string("0000");
        } else if (lower < 10000000) {
            write_string("000");
        } else if (lower < 100000000) {
            write_string("00");
        } else if (lower < 1000000000) {
            write_string("0");
        }
        write_string(buffer);
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
    
    write_string(&buffer[0]);
}

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
}