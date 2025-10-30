#include "logger.h"
#include "terminal.h"
#include "io.h"
#include "libc.h"
#include <stdarg.h>

atomic_uint_fast32_t interrupt_guard_counter = 0;
static LogBuffer g_log_buffer;
static Logger g_logger;

int logger_buffer_is_full(void) {
    return g_log_buffer.count >= LOG_BUFFER_SIZE;
}

int logger_buffer_is_empty(void) {
    return g_log_buffer.count == 0;
}

void logger_buffer_push(const LogEntry* entry) {
    if (logger_buffer_is_full()) {
        g_log_buffer.head = (g_log_buffer.head + 1) % LOG_BUFFER_SIZE;
        g_log_buffer.count--;
    }

    g_log_buffer.buffer[g_log_buffer.tail] = *entry;

    g_log_buffer.tail = (g_log_buffer.tail + 1) % LOG_BUFFER_SIZE;
    g_log_buffer.count++;
}

int logger_buffer_pop(LogEntry* entry) {
    if (logger_buffer_is_empty()) {
        return 0;
    }

    *entry = g_log_buffer.buffer[g_log_buffer.head];

    g_log_buffer.head = (g_log_buffer.head + 1) % LOG_BUFFER_SIZE;
    g_log_buffer.count--;
    
    return 1; 
}

void interrupt_guard_acquire(void) {
    __asm__ volatile ("cli");

    atomic_fetch_add(&interrupt_guard_counter, 1);
}

void interrupt_guard_release(void) {
    uint32_t new_count = atomic_fetch_sub(&interrupt_guard_counter, 1) - 1;

    if (new_count == 0) {
        __asm__ volatile ("sti");
    }
}

void logger_init(void) {
    g_log_buffer.head = 0;
    g_log_buffer.tail = 0;
    g_log_buffer.count = 0;

    g_logger.buffer = &g_log_buffer;
    g_logger.default_level = LOG_LEVEL_INFO;
    
    output_string("Logger initialized!\n");
}

LogLevel logger_get_module_level(const char* module) {
    return g_logger.default_level;
}

void logger_set_module_level(const char* module, LogLevel level) {
    g_logger.default_level = level;
}

static const char* log_level_to_string(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_DEBUG:   return "DEBUG";
        case LOG_LEVEL_INFO:    return "INFO";
        case LOG_LEVEL_WARNING: return "WARNING";
        case LOG_LEVEL_ERROR:   return "ERROR";
        default:                return "UNKNOWN";
    }
}

static int simple_format_string(char* buffer, size_t buffer_size, const char* format, va_list args) {
    const char* src = format;
    char* dst = buffer;
    size_t written = 0;
    
    while (*src && written < buffer_size - 1) {
        if (*src == '%' && *(src + 1)) {
            src++;
            switch (*src) {
                case 's': {
                    char* str = va_arg(args, char*);
                    if (str) {
                        while (*str && written < buffer_size - 1) {
                            *dst++ = *str++;
                            written++;
                        }
                    }
                    break;
                }
                case 'd': {
                    int num = va_arg(args, int);
                    char num_str[16];  
                    int_to_string(num, num_str);
                    int i = 0;
                    while (num_str[i] && written < buffer_size - 1) {
                        *dst++ = num_str[i++];
                        written++;
                    }
                    break;
                }
                case 'x': {
                    unsigned int num = va_arg(args, unsigned int);
                    char num_str[16];
                    // Convert to hex by processing nibbles
                    if (num == 0) {
                        if (written < buffer_size - 1) {
                            *dst++ = '0';
                            written++;
                        }
                    } else {
                        char temp[16];
                        int pos = 0;
                        unsigned int n = num;
                        
                        while (n > 0) {
                            int digit = n & 0xF;
                            if (digit < 10) {
                                temp[pos++] = '0' + digit;
                            } else {
                                temp[pos++] = 'a' + (digit - 10);
                            }
                            n >>= 4;
                        }

                        for (int j = pos - 1; j >= 0; j--) {
                            if (written < buffer_size - 1) {
                                *dst++ = temp[j];
                                written++;
                            }
                        }
                    }
                    break;
                }
                default:
                    if (written < buffer_size - 1) {
                        *dst++ = *src;
                        written++;
                    }
                    break;
            }
        } else {
            *dst++ = *src;
            written++;
        }
        src++;
    }
    
    *dst = '\0';
    return written;
}

void logger_log(LogLevel level, const char* module, const char* format, ...) {
    LogLevel module_level = logger_get_module_level(module);
    if (level < module_level) {
        return; 
    }

    interrupt_guard_acquire();

    LogEntry entry;
    entry.level = level;

    int i;
    for (i = 0; i < sizeof(entry.module) - 1 && module[i] != '\0'; i++) {
        entry.module[i] = module[i];
    }
    entry.module[i] = '\0';

    va_list args;
    va_start(args, format);
    simple_format_string(entry.message, sizeof(entry.message), format, args);
    va_end(args);

    logger_buffer_push(&entry);

    interrupt_guard_release();
}

void logger_debug(const char* module, const char* format, ...) {
    LogLevel module_level = logger_get_module_level(module);
    if (LOG_LEVEL_DEBUG >= module_level) {
        va_list args;
        va_start(args, format);
        
        interrupt_guard_acquire();
        
        LogEntry entry;
        entry.level = LOG_LEVEL_DEBUG;
 
        int i;
        for (i = 0; i < sizeof(entry.module) - 1 && module[i] != '\0'; i++) {
            entry.module[i] = module[i];
        }
        entry.module[i] = '\0';
        
        simple_format_string(entry.message, sizeof(entry.message), format, args);
        
        logger_buffer_push(&entry);
        interrupt_guard_release();
        
        va_end(args);
    }
}

void logger_info(const char* module, const char* format, ...) {
    LogLevel module_level = logger_get_module_level(module);
    if (LOG_LEVEL_INFO >= module_level) {
        va_list args;
        va_start(args, format);
        
        interrupt_guard_acquire();
        
        LogEntry entry;
        entry.level = LOG_LEVEL_INFO;

        int i;
        for (i = 0; i < sizeof(entry.module) - 1 && module[i] != '\0'; i++) {
            entry.module[i] = module[i];
        }
        entry.module[i] = '\0';
        
        simple_format_string(entry.message, sizeof(entry.message), format, args);
        
        logger_buffer_push(&entry);
        interrupt_guard_release();
        
        va_end(args);
    }
}

void logger_warning(const char* module, const char* format, ...) {
    LogLevel module_level = logger_get_module_level(module);
    if (LOG_LEVEL_WARNING >= module_level) {
        va_list args;
        va_start(args, format);
        
        interrupt_guard_acquire();
        
        LogEntry entry;
        entry.level = LOG_LEVEL_WARNING;
        
        int i;
        for (i = 0; i < sizeof(entry.module) - 1 && module[i] != '\0'; i++) {
            entry.module[i] = module[i];
        }
        entry.module[i] = '\0';
        
        simple_format_string(entry.message, sizeof(entry.message), format, args);
        
        logger_buffer_push(&entry);
        interrupt_guard_release();
        
        va_end(args);
    }
}

void logger_error(const char* module, const char* format, ...) {
    LogLevel module_level = logger_get_module_level(module);
    if (LOG_LEVEL_ERROR >= module_level) {
        va_list args;
        va_start(args, format);
        
        interrupt_guard_acquire();
        
        LogEntry entry;
        entry.level = LOG_LEVEL_ERROR;

        int i;
        for (i = 0; i < sizeof(entry.module) - 1 && module[i] != '\0'; i++) {
            entry.module[i] = module[i];
        }
        entry.module[i] = '\0';
        
        simple_format_string(entry.message, sizeof(entry.message), format, args);
        
        logger_buffer_push(&entry);
        interrupt_guard_release();
        
        va_end(args);
    }
}

void logger_service(void) {
    LogEntry entry;

    while (logger_buffer_pop(&entry)) {
        output_string("[");
        output_string((char*)log_level_to_string(entry.level));
        output_string("] ");
        output_string(entry.module);
        output_string(": ");
        output_string(entry.message);
        output_string("\n");
    }
}