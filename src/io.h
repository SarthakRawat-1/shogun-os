#ifndef IO_H
#define IO_H

#include <stdint.h>

uint8_t in_b(uint16_t port);
void out_b(uint16_t port, uint8_t value);

void init_serial();
int serial_is_transmit_empty();
void write_serial(char c);
void write_serial_string(const char* str);

void exit_qemu(uint8_t exit_code);

#endif