#include "rtc.h"
#include "io.h"
#include "terminal.h"
#include "memory.h"
#include "idt.h"
#include <stdbool.h>

// Global tick counter for monotonic clock
volatile uint32_t system_tick_count = 0;

RTCDriver* init_rtc() {
    PortHandle* control_port = request_port(CMOS_CONTROL_PORT);
    if (control_port == NULL) {
        output_string("Failed to acquire CMOS control port\n");
        return NULL;
    }
    
    PortHandle* data_port = request_port(CMOS_DATA_PORT);
    if (data_port == NULL) {
        output_string("Failed to acquire CMOS data port\n");
        release_port(control_port);
        return NULL;
    }
    
    RTCDriver* rtc = (RTCDriver*)malloc(sizeof(RTCDriver));
    if (rtc == NULL) {
        output_string("Failed to allocate memory for RTC driver\n");
        release_port(control_port);
        release_port(data_port);
        return NULL;
    }
    
    rtc->control_port = control_port;
    rtc->data_port = data_port;
    rtc->nmi_enabled = true;  

    set_data_format(rtc);
    
    return rtc;
}

uint8_t read_cmos_register(RTCDriver* rtc, uint8_t reg) {
    if (rtc == NULL) {
        return 0;
    }

    uint8_t nmi_mask = rtc->nmi_enabled ? 0x00 : NMI_DISABLE_MASK;

    uint8_t reg_select = reg | nmi_mask;
    write_port_b(rtc->control_port, reg_select);

    uint8_t value = read_port_b(rtc->data_port);
    
    return value;
}

void write_cmos_register(RTCDriver* rtc, uint8_t reg, uint8_t value) {
    if (rtc == NULL) {
        return;
    }

    uint8_t nmi_mask = rtc->nmi_enabled ? 0x00 : NMI_DISABLE_MASK;

    uint8_t reg_select = reg | nmi_mask;
    write_port_b(rtc->control_port, reg_select);

    write_port_b(rtc->data_port, value);
}

void set_data_format(RTCDriver* rtc) {
    if (rtc == NULL) {
        return;
    }

    uint8_t status_reg_b = read_cmos_register(rtc, CMOS_REG_B);

    status_reg_b |= (1 << 1) | (1 << 2);  // Enables 24 hour mode and binary format

    write_cmos_register(rtc, CMOS_REG_B, status_reg_b);
}

bool update_in_progress(RTCDriver* rtc) {
    if (rtc == NULL) {
        return false;
    }

    uint8_t status_reg_a = read_cmos_register(rtc, CMOS_REG_A);
    return (status_reg_a & (1 << 7)) != 0;
}

typedef struct {
    uint8_t seconds, minutes, hours;
    uint8_t (*read_op)(RTCDriver*, uint8_t*);
    void (*write_op)(RTCDriver*, uint8_t, uint8_t*);
} OperationData;

static uint8_t read_time_guarded_op(RTCDriver* rtc, void* data) {
    uint8_t* time_data = (uint8_t*)data;
    time_data[0] = read_cmos_register(rtc, CMOS_REG_SECONDS);
    time_data[1] = read_cmos_register(rtc, CMOS_REG_MINUTES);
    time_data[2] = read_cmos_register(rtc, CMOS_REG_HOURS);
    return 0;
}

static uint8_t write_time_guarded_op(RTCDriver* rtc, void* data) {
    uint8_t* time_data = (uint8_t*)data;
    write_cmos_register(rtc, CMOS_REG_SECONDS, time_data[0]);
    write_cmos_register(rtc, CMOS_REG_MINUTES, time_data[1]);
    write_cmos_register(rtc, CMOS_REG_HOURS, time_data[2]);
    return 0;
}

int update_guarded_op(RTCDriver* rtc, uint8_t (*op_func)(RTCDriver*, void*), void* op_data) {
    if (rtc == NULL || op_func == NULL) {
        return -1;
    }
    
    while (1) {  
        while (update_in_progress(rtc)) {
        }

        uint8_t result = op_func(rtc, op_data);

        if (!update_in_progress(rtc)) {
            return result;
        }
    }
}

int read_rtc_time(RTCDriver* rtc, uint8_t* seconds, uint8_t* minutes, uint8_t* hours) {
    if (rtc == NULL || seconds == NULL || minutes == NULL || hours == NULL) {
        return -1;
    }
    
    uint8_t time_data[3];

    int result = update_guarded_op(rtc, read_time_guarded_op, time_data);

    *seconds = time_data[0];
    *minutes = time_data[1];
    *hours = time_data[2];
    
    return result;
}

int write_rtc_time(RTCDriver* rtc, uint8_t seconds, uint8_t minutes, uint8_t hours) {
    if (rtc == NULL) {
        return -1;
    }
    
    uint8_t time_data[3];
    time_data[0] = seconds;
    time_data[1] = minutes;
    time_data[2] = hours;

    int result = update_guarded_op(rtc, write_time_guarded_op, time_data);
    
    return result;
}

int enable_rtc_interrupts(RTCDriver* rtc, interrupt_handler_t handler) {
    if (rtc == NULL) {
        return -1;
    }

    // RTC interrupt is connected to IRQ 8, which after remapping goes to vector 72
    IrqId rtc_irq;
    rtc_irq.type = IRQ_PIC2;
    rtc_irq.index = 0;

    int result = register_interrupt_handler_irq(rtc_irq, handler);
    if (result != 0) {
        output_string("Failed to register RTC interrupt handler\n");
        return -1;
    }

    // Set up periodic interrupts in CMOS register A for 256 Hz
    uint8_t reg_a = read_cmos_register(rtc, CMOS_REG_A);

    reg_a = (reg_a & 0xF0) | 0x08;  
    write_cmos_register(rtc, CMOS_REG_A, reg_a);

    // Enable periodic interrupts in CMOS register B
    uint8_t reg_b = read_cmos_register(rtc, CMOS_REG_B);
    reg_b |= 0x40;  // Set bit 6 (PIE - Periodic Interrupt Enable)
    write_cmos_register(rtc, CMOS_REG_B, reg_b);

    pic_unmask_irq(8);

    return 0;
}

int disable_rtc_interrupts(RTCDriver* rtc) {
    if (rtc == NULL) {
        return -1;
    }
    
    // Disable periodic interrupts in CMOS register B
    uint8_t reg_b = read_cmos_register(rtc, CMOS_REG_B);
    reg_b &= ~0x40;  
    write_cmos_register(rtc, CMOS_REG_B, reg_b);
    
    IrqId rtc_irq;
    rtc_irq.type = IRQ_PIC2;  
    rtc_irq.index = 0;        
    
    int result = unregister_interrupt_handler_irq(rtc_irq);
    return result;
}

void clear_rtc_interrupt(RTCDriver* rtc) {
    if (rtc == NULL) {
        return;
    }

    read_cmos_register(rtc, CMOS_REG_C);
}

void acknowledge_rtc_interrupt(void) {
    PortHandle* control_port = request_port(CMOS_CONTROL_PORT);
    if (control_port != NULL) {
        PortHandle* data_port = request_port(CMOS_DATA_PORT);
        if (data_port != NULL) {
            uint8_t nmi_mask = 0x00; 
            uint8_t reg_select = CMOS_REG_C | nmi_mask;
            write_port_b(control_port, reg_select);
            uint8_t value = read_port_b(data_port);  

            release_port(data_port);
        }
        release_port(control_port);
    }
}

uint32_t get_system_ticks(void) {
    return system_tick_count;
}

void sleep_ticks(uint32_t ticks) {
    if (ticks == 0) return;

    uint32_t start_tick = system_tick_count;
    uint32_t target_tick = start_tick + ticks;

    // Enable interrupts globally before entering sleep loop
    __asm__ volatile ("sti");

    uint32_t current_tick = start_tick;
    uint32_t loop_counter = 0;
    const uint32_t MAX_LOOP_COUNT = 1000000; 

    while (current_tick < target_tick && loop_counter < MAX_LOOP_COUNT) {
        __asm__ volatile ("hlt");  
        current_tick = system_tick_count;
        loop_counter++;
    }

    if (loop_counter >= MAX_LOOP_COUNT) {
        output_string("WARNING: Sleep function timeout - tick counter may not be working\n");
    }
}

void sleep_seconds(uint32_t seconds) {
    uint32_t ticks = seconds * 256;
    sleep_ticks(ticks);
}