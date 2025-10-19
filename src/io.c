#include "io.h"
#include "port_manager.h"
#include <stdint.h>
#include <stddef.h>

#define SERIAL_COM1 0x3F8
#define SERIAL_DATA_PORT(base) (base)
#define SERIAL_FIFO_COMMAND_PORT(base) (base + 2)
#define SERIAL_LINE_COMMAND_PORT(base) (base + 3)
#define SERIAL_MODEM_COMMAND_PORT(base) (base + 4)
#define SERIAL_LINE_STATUS_PORT(base) (base + 5)

#define SERIAL_LINE_ENABLE_DLAB 0x80

#define SERIAL_LINE_STATUS_TRANSMIT_EMPTY 0x20
#define SERIAL_LINE_STATUS_EMPTY 0x40

uint8_t read_port_b(PortHandle* handle) {
    if (handle == NULL) {
        return 0;
    }
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a" (ret) : "Nd" (handle->port));
    return ret;
}

void write_port_b(PortHandle* handle, uint8_t value) {
    if (handle == NULL) {
        return;
    }
    __asm__ volatile ("outb %0, %1" : : "a" (value), "Nd" (handle->port));
}

uint8_t in_b(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a" (ret) : "Nd" (port));
    return ret;
}

void out_b(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a" (value), "Nd" (port));
}

void init_serial() {
    PortHandle* serial_handle = request_port(SERIAL_COM1);
    if (serial_handle == NULL) {
        return;
    }

    uint16_t port = serial_handle->port;

    out_b(port + 1, 0x00);

    out_b(port + 3, SERIAL_LINE_ENABLE_DLAB);

    out_b(port + 0, 0x03);
    out_b(port + 1, 0x00);

    out_b(port + 3, 0x03);

    out_b(port + 2, 0xC7);

    out_b(port + 4, 0x0B);

    out_b(port + 4, 0x1E);

    out_b(port + 0, 0xAE);

    if (in_b(port + 0) != 0xAE) {

    }

    out_b(port + 4, 0x0F);
}

int serial_is_transmit_empty() {
    return in_b(SERIAL_LINE_STATUS_PORT(SERIAL_COM1)) & SERIAL_LINE_STATUS_EMPTY;
}

void write_serial(char c) {
    while (serial_is_transmit_empty() == 0);
    
    out_b(SERIAL_DATA_PORT(SERIAL_COM1), c);
}

void write_serial_string(const char* str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            write_serial('\r');  
        }
        write_serial(str[i]);
    }
}

void exit_qemu(uint8_t exit_code) {
    out_b(0x402, exit_code);
    out_b(0x80, exit_code);

    while(1) {
        __asm__ volatile ("hlt");
    }
}