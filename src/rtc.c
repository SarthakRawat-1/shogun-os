#include "rtc.h"
#include "io.h"
#include "terminal.h"
#include "memory.h"
#include "idt.h"
#include <stdbool.h>

// Global tick counter for monotonic clock (kept for backward compatibility)
volatile uint32_t system_tick_count = 0;

MonotonicTime* monotonic_time = NULL;
WakeUpList* wake_up_list = NULL;

// Initialize global monotonic time
void monotonic_time_init_global(void) {
    if (monotonic_time == NULL) {
        monotonic_time = (MonotonicTime*)malloc(sizeof(MonotonicTime));
        if (monotonic_time != NULL) {
            atomic_store(&monotonic_time->tick_count, 0);
        }
    }
}

uint32_t monotonic_time_get_ticks_global(void) {
    if (monotonic_time != NULL) {
        return atomic_load(&monotonic_time->tick_count);
    }
    return 0;
}

void monotonic_time_increment_global(void) {
    if (monotonic_time != NULL) {
        atomic_fetch_add(&monotonic_time->tick_count, 1);
    }
}

// Initialize wake-up list
void wake_up_list_init(void) {
    if (wake_up_list == NULL) {
        wake_up_list = (WakeUpList*)malloc(sizeof(WakeUpList));
        if (wake_up_list != NULL) {
            wake_up_list->entries = NULL;
            atomic_store(&wake_up_list->entry_count, 0);
        }
    }
}

void wake_up_list_add(uint32_t wake_up_tick, void (*callback)(void* data), void* callback_data) {
    if (wake_up_list == NULL) return;

    WakeUpEntry* new_entry = (WakeUpEntry*)malloc(sizeof(WakeUpEntry));
    if (new_entry == NULL) return;

    new_entry->wake_up_tick = wake_up_tick;
    new_entry->callback = callback;
    new_entry->callback_data = callback_data;
    new_entry->next = wake_up_list->entries;
    wake_up_list->entries = new_entry;

    atomic_fetch_add(&wake_up_list->entry_count, 1);
}

void wake_up_list_check_and_execute(void) {
    if (wake_up_list == NULL) return;

    uint32_t current_tick = monotonic_time_get_ticks_global();
    WakeUpEntry* current = wake_up_list->entries;
    WakeUpEntry** prev_ptr = &wake_up_list->entries;

    while (current) {
        if (current->wake_up_tick <= current_tick) {
            if (current->callback) {
                current->callback(current->callback_data);
            }

            *prev_ptr = current->next;
            WakeUpEntry* to_free = current;
            current = current->next;
            free(to_free);
            atomic_fetch_sub(&wake_up_list->entry_count, 1);
        } else {
            prev_ptr = &current->next;
            current = current->next;
        }
    }
}

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

// Callback function to wake up the executor when sleep is complete
static void sleep_callback(void* data) {
    (void)data; 
    executor_wake_up();
}

void sleep_seconds(uint32_t seconds) {
    uint32_t ticks = seconds * 256;
    uint32_t target_tick = monotonic_time_get_ticks_global() + ticks;

    // Add to wake-up list to wake up the executor when sleep is complete
    wake_up_list_add(target_tick, sleep_callback, NULL);

    // Now we wait by letting the executor handle it
    // In a real scenario, this would be part of a future that waits
    for (volatile uint32_t i = 0; monotonic_time_get_ticks_global() < target_tick; i++) {
        // Busy wait for demonstration
        // In async system, this would be handled by the executor
    }
}

void sleep_seconds_async(uint32_t seconds) {
    uint32_t ticks = seconds * 256;
    Future* sleep_future = sleep_future_create(ticks);
    if (sleep_future != NULL) {
        Executor* executor = get_global_executor();
        executor_spawn(executor, sleep_future);
    }
}

// Async RTC future implementation
static FutureState async_rtc_future_poll(Future* future, void* context) {
    AsyncRTCFuture* async_rtc = (AsyncRTCFuture*)future;

    // For simplicity, we'll consider the RTC read as immediately available
    // In a real implementation, this would check if the RTC is ready
    if (async_rtc->rtc != NULL && async_rtc->seconds != NULL &&
        async_rtc->minutes != NULL && async_rtc->hours != NULL) {

        // Perform the RTC read
        int result = read_rtc_time(async_rtc->rtc, async_rtc->seconds,
                                  async_rtc->minutes, async_rtc->hours);

        // If read was successful, mark as ready
        if (result != -1) {
            return FUTURE_READY;
        }
    }

    return FUTURE_PENDING;
}

static void async_rtc_future_cleanup(Future* future) {
    // Free the async RTC future
    free(future);
}

static const FutureVTable async_rtc_future_vtable = {
    .poll = async_rtc_future_poll,
    .cleanup = async_rtc_future_cleanup
};

Future* async_rtc_read_time_create(RTCDriver* rtc, uint8_t* seconds, uint8_t* minutes, uint8_t* hours) {
    AsyncRTCFuture* async_rtc = (AsyncRTCFuture*)malloc(sizeof(AsyncRTCFuture));
    if (!async_rtc) {
        return NULL;
    }

    async_rtc->base.vtable = &async_rtc_future_vtable;
    async_rtc->base.is_completed = false;
    async_rtc->base.waker = NULL;
    async_rtc->rtc = rtc;
    async_rtc->seconds = seconds;
    async_rtc->minutes = minutes;
    async_rtc->hours = hours;

    return (Future*)async_rtc;
}