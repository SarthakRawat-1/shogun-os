#include "port_manager.h"
#include "terminal.h"
#include "memory.h"
#include <stdbool.h>

#define MAX_TRACKED_PORTS 64

static PortHandle port_registry[MAX_TRACKED_PORTS];
static bool initialized = false;

void init_port_manager() {
    if (initialized) {
        return; 
    }

    for (int i = 0; i < MAX_TRACKED_PORTS; i++) {
        port_registry[i].in_use = false;
        port_registry[i].port = 0;
        port_registry[i].next = NULL;
    }
    
    initialized = true;
}

PortHandle* request_port(uint16_t port) {
    if (!initialized) {
        init_port_manager();
    }

    for (int i = 0; i < MAX_TRACKED_PORTS; i++) {
        if (port_registry[i].in_use && port_registry[i].port == port) {
            return NULL;
        }
    }

    for (int i = 0; i < MAX_TRACKED_PORTS; i++) {
        if (!port_registry[i].in_use) {
            port_registry[i].port = port;
            port_registry[i].in_use = true;
            return &port_registry[i];
        }
    }
    
    return NULL;
}

void release_port(PortHandle* handle) {
    if (handle != NULL) {

        if (handle >= port_registry && handle < port_registry + MAX_TRACKED_PORTS) {

            if (handle->in_use) {
                handle->in_use = false;
            }
        }
    }
}

bool is_port_in_use(uint16_t port) {
    for (int i = 0; i < MAX_TRACKED_PORTS; i++) {
        if (port_registry[i].in_use && port_registry[i].port == port) {
            return true;
        }
    }
    return false;
}