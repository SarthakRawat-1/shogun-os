#ifndef RTC_H
#define RTC_H

#include <stdint.h>
#include <stdbool.h>
#include "port_manager.h"
#include "idt.h"  

#define CMOS_REG_SECONDS        0x00
#define CMOS_REG_MINUTES        0x02
#define CMOS_REG_HOURS          0x04
#define CMOS_REG_WEEK_DAY       0x06
#define CMOS_REG_DAY            0x07
#define CMOS_REG_MONTH          0x08
#define CMOS_REG_YEAR           0x09
#define CMOS_REG_A              0x0A
#define CMOS_REG_B              0x0B
#define CMOS_REG_C              0x0C
#define CMOS_REG_D              0x0D

#define CMOS_CONTROL_PORT       0x70
#define CMOS_DATA_PORT          0x71

#define NMI_DISABLE_MASK        0x80

typedef struct {
    PortHandle* control_port;  
    PortHandle* data_port;     
    bool nmi_enabled;          
} RTCDriver;

RTCDriver* init_rtc();

uint8_t read_cmos_register(RTCDriver* rtc, uint8_t reg);

void write_cmos_register(RTCDriver* rtc, uint8_t reg, uint8_t value);

void set_data_format(RTCDriver* rtc);

bool update_in_progress(RTCDriver* rtc);

int update_guarded_op(RTCDriver* rtc, uint8_t (*op_func)(RTCDriver*, void*), void* op_data);

int read_rtc_time(RTCDriver* rtc, uint8_t* seconds, uint8_t* minutes, uint8_t* hours);

int write_rtc_time(RTCDriver* rtc, uint8_t seconds, uint8_t minutes, uint8_t hours);

int enable_rtc_interrupts(RTCDriver* rtc, interrupt_handler_t handler);
int disable_rtc_interrupts(RTCDriver* rtc);
void clear_rtc_interrupt(RTCDriver* rtc);

void acknowledge_rtc_interrupt(void);

extern volatile uint32_t system_tick_count;

uint32_t get_system_ticks(void);

void sleep_ticks(uint32_t ticks);

void sleep_seconds(uint32_t seconds);

#endif