#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>
#include <stdatomic.h>
#include "terminal.h"  

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARNING = 2,
    LOG_LEVEL_ERROR = 3
} LogLevel;

#define MAX_LOG_MESSAGE_LENGTH 256
#define LOG_BUFFER_SIZE 64

typedef struct {
    LogLevel level;
    char message[MAX_LOG_MESSAGE_LENGTH];
    char module[64];  
} LogEntry;

typedef struct {
    LogEntry buffer[LOG_BUFFER_SIZE];
    volatile uint32_t head; 
    volatile uint32_t tail;  
    volatile uint32_t count; 
} LogBuffer;

extern atomic_uint_fast32_t interrupt_guard_counter;

typedef struct {
    LogBuffer* buffer;
    LogLevel default_level;
} Logger;

void logger_init(void);

void interrupt_guard_acquire(void);

void interrupt_guard_release(void);

void logger_log(LogLevel level, const char* module, const char* format, ...);

void logger_service(void);

void logger_set_module_level(const char* module, LogLevel level);

LogLevel logger_get_module_level(const char* module);

void logger_buffer_push(const LogEntry* entry);

int logger_buffer_pop(LogEntry* entry);

int logger_buffer_is_full(void);

int logger_buffer_is_empty(void);

void logger_debug(const char* module, const char* format, ...);
void logger_info(const char* module, const char* format, ...);
void logger_warning(const char* module, const char* format, ...);
void logger_error(const char* module, const char* format, ...);

#define LOG_DEBUG(module, fmt, ...) logger_debug(module, fmt, ##__VA_ARGS__)
#define LOG_INFO(module, fmt, ...)  logger_info(module, fmt, ##__VA_ARGS__)
#define LOG_WARNING(module, fmt, ...) logger_warning(module, fmt, ##__VA_ARGS__)
#define LOG_ERROR(module, fmt, ...) logger_error(module, fmt, ##__VA_ARGS__)

#define LOG_DEBUG_HERE(fmt, ...) logger_debug(__FILE__, fmt, ##__VA_ARGS__)
#define LOG_INFO_HERE(fmt, ...)  logger_info(__FILE__, fmt, ##__VA_ARGS__)
#define LOG_WARNING_HERE(fmt, ...) logger_warning(__FILE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR_HERE(fmt, ...) logger_error(__FILE__, fmt, ##__VA_ARGS__)

#endif