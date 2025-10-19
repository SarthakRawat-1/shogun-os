#include "rtc.h"
#include "io.h"
#include "terminal.h"
#include "memory.h"
#include <stdbool.h>

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

    status_reg_b |= (1 << 1) | (1 << 2);  

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