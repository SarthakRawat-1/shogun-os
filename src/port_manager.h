#ifndef PORT_MANAGER_H
#define PORT_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#define PORT_SERIAL_COM1 0x3F8
#define PORT_CMOS_CONTROL 0x70
#define PORT_CMOS_DATA 0x71

// PortHandle objects are managed internally by the port manager.
typedef struct PortHandle {
    uint16_t port;
    bool in_use;
    struct PortHandle* next;
} PortHandle;

void init_port_manager();

PortHandle* request_port(uint16_t port);

void release_port(PortHandle* handle);

bool is_port_in_use(uint16_t port);

#endif