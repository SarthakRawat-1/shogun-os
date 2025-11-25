#include <stdint.h>
#include <stddef.h>
#include "terminal.h"
#include "libc.h"
#include "multiboot.h"
#include "memory.h"
#include "io.h"
#include "test.h"
#include "port_manager.h"
#include "rtc.h"
#include "gdt.h"
#include "idt.h"
#include "logger.h"

void run_rtc_tests(void);  

static volatile uint32_t rtc_interrupt_count = 0;

// Global system RTC driver for continuous operation
static RTCDriver* system_rtc_instance = NULL;

void rtc_interrupt_handler(void) {
    system_tick_count++;

    if (system_rtc_instance != NULL) {
        clear_rtc_interrupt(system_rtc_instance);
    } else {
        acknowledge_rtc_interrupt();
    }

    rtc_interrupt_count++;

    // Debug: Print every 256 ticks (1 second)
    if (system_tick_count % 256 == 0) {
        output_string(".");  
    }

    pic_send_eoi(0x48);
}

static int custom_handler_called = 0;

void custom_interrupt_handler(void) {
    custom_handler_called++;
    output_string("Custom interrupt 0x81 handler called! Count: ");
    put_u32(custom_handler_called);
    output_string("\n");
}

void kernel_main(uint32_t magic, uint32_t multiboot_info_ptr) {
    init_output();
    
    output_string("hi shogun from c - Test Mode\n");

    output_string("Initializing logger...\n");
    logger_init();
    output_string("Logger initialized successfully!\n");

    LOG_DEBUG_HERE("This is a debug message");
    LOG_INFO_HERE("This is an info message");
    LOG_WARNING_HERE("This is a warning message");
    LOG_ERROR_HERE("This is an error message");
    
    output_string("Logger buffer populated with test messages, now servicing...\n");

    logger_service();
    
    output_string("Initializing GDT...\n");
    gdt_init();
    output_string("GDT initialized successfully!\n");
    
    print_gdt_info(); 
    
    output_string("Initializing IDT...\n");
    idt_init();
    output_string("IDT initialized successfully!\n");
    
    print_idt_info(); 
    
    output_string("Initializing PIC...\n");
    pic_init();  
    output_string("PIC initialized successfully!\n");
    
    output_string("Remapping PIC...\n");
    pic_remap();  
    output_string("PIC remapped successfully!\n");

    output_string("Testing interrupt 0x80...\n");
    __asm__ volatile ("int $0x80");
    output_string("Interrupt 0x80 test completed!\n");

    output_string("Testing divide by zero exception handling...\n");
    output_string("Setting up divide by zero test (this should trigger exception handler)...\n");
 
    LOG_INFO_HERE("Kernel initialization is almost complete");
    
    output_string("Divide by zero test completed (skipped to prevent crash during normal execution)!\n");
    
    output_string("Initializing memory allocator for tests...\n");
    init_allocator(multiboot_info_ptr);
    
    output_string("Initializing port manager...\n");
    init_port_manager();
    
    run_memory_tests();
    
    output_string("\nRunning RTC tests...\n");
    run_rtc_tests();


    output_string("\nDynamic Interrupt Registration System Active!\n");
    output_string("RTC driver successfully registered for periodic interrupts using the new system.\n");

    output_string("Setting up system-wide periodic RTC interrupts...\n");
    system_rtc_instance = init_rtc();
    if (system_rtc_instance != NULL) {
        int rtc_result = enable_rtc_interrupts(system_rtc_instance, rtc_interrupt_handler);
        if (rtc_result == 0) {
            output_string("Periodic RTC interrupts enabled successfully for system clock!\n");
            output_string("System tick counter will now increment with each RTC interrupt.\n");
        } else {
            output_string("Failed to enable periodic RTC interrupts\n");
        }
    } else {
        output_string("Failed to initialize RTC for system clock\n");
    }
    
    output_string("Registering custom handler for interrupt 0x81...\n");

    int result = register_interrupt_handler(0x81, custom_interrupt_handler);
    if (result == 0) {
        output_string("Successfully registered custom handler for interrupt 0x81!\n");

        output_string("Triggering interrupt 0x81...\n");
        __asm__ volatile ("int $0x81");
        
        output_string("Triggering interrupt 0x81 again...\n");
        __asm__ volatile ("int $0x81");

        output_string("Custom handler was called ");
        put_u32(custom_handler_called);
        output_string(" time(s)\n");

        unregister_interrupt_handler(0x81);
        output_string("Custom handler unregistered successfully!\n");
    } else {
        output_string("Failed to register custom handler\n");
    }

    logger_service();

    __asm__ volatile ("sti");

    output_string("\nTesting sleep functionality with monotonic clock...\n");
    output_string("Current system ticks: ");
    put_u32(get_system_ticks());
    output_string("\n");

    output_string("Sleeping for 2 seconds (512 ticks at 256Hz)...\n");
    uint32_t ticks_before_sleep = get_system_ticks();
    sleep_seconds(2);
    uint32_t ticks_after_sleep = get_system_ticks();

    output_string("Woke up! Ticks before: ");
    put_u32(ticks_before_sleep);
    output_string(", Ticks after: ");
    put_u32(ticks_after_sleep);
    output_string(", Elapsed: ");
    put_u32(ticks_after_sleep - ticks_before_sleep);
    output_string("\n");

    output_string("Sleep functionality demonstrated successfully!\n");

    exit_after_all_tests(0);
}

void test_divide_by_zero_safely(void) {
    output_string("About to trigger divide by zero...\n");
    // Would cause a system crash, so won't actually do it during normal execution
    // int zero = 0;
    // volatile int result = 1 / zero; // Use volatile to prevent optimization
}

TEST(memory_alloc_basic) {
    char* ptr = (char*)malloc(10);
    ASSERT(ptr != NULL, "Basic allocation should succeed");
    if (ptr != NULL) {
        free(ptr);
    }
}

TEST(memory_alloc_zero) {
    char* ptr = (char*)malloc(0);
    if (ptr != NULL) {
        free(ptr);
    }
}

TEST(memory_alloc_and_free) {
    int* numbers = (int*)malloc(5 * sizeof(int));
    ASSERT(numbers != NULL, "Array allocation should succeed");
    
    if (numbers != NULL) {
        for (int i = 0; i < 5; i++) {
            numbers[i] = i * 10;
        }

        for (int i = 0; i < 5; i++) {
            char msg[64];
            ASSERT_EQUAL(i * 10, numbers[i], "Array values should be correct");
        }
        
        free(numbers);
    }
}

TEST(memory_multiple_alloc_free) {
    char* str1 = (char*)malloc(20);
    int* arr1 = (int*)malloc(10 * sizeof(int));
    char* str2 = (char*)malloc(30);
    
    ASSERT(str1 != NULL, "First string allocation should succeed");
    ASSERT(arr1 != NULL, "Array allocation should succeed");
    ASSERT(str2 != NULL, "Second string allocation should succeed");
    
    if (str1 && arr1 && str2) {
        str1[0] = 'A';
        arr1[0] = 100;
        str2[0] = 'B';
        
        ASSERT_EQUAL('A', str1[0], "First string should hold value");
        ASSERT_EQUAL(100, arr1[0], "Array should hold value");
        ASSERT_EQUAL('B', str2[0], "Second string should hold value");
        
        free(arr1);
        free(str1);
        free(str2);
    }
}

TEST(rtc_basic_init) {
    output_string("Attempting to initialize RTC...\n");
    RTCDriver* rtc = init_rtc();
    ASSERT(rtc != NULL, "RTC initialization should succeed");
    
    if (rtc != NULL) {
        output_string("RTC initialized successfully, attempting to read time...\n");

        uint8_t seconds, minutes, hours;
        int result = read_rtc_time(rtc, &seconds, &minutes, &hours);
        ASSERT(result != -1, "Reading RTC time should succeed");

        output_string("RTC Time read: ");
        put_u32(hours);
        output_string(":");
        put_u32(minutes);
        output_string(":");
        put_u32(seconds);
        output_string("\n");
        

        if (hours <= 23 && minutes <= 59 && seconds <= 59) {
            uint8_t new_hour = (hours > 0) ? hours - 1 : 23; 
            result = write_rtc_time(rtc, seconds, minutes, new_hour);
            ASSERT(result != -1, "Writing RTC time should succeed");
            output_string("RTC time write completed\n");

            uint8_t verify_sec, verify_min, verify_hour;
            result = read_rtc_time(rtc, &verify_sec, &verify_min, &verify_hour);
            ASSERT(result != -1, "Reading RTC time after write should succeed");
            output_string("RTC Time after write attempt: ");
            put_u32(verify_hour);
            output_string(":");
            put_u32(verify_min);
            output_string(":");
            put_u32(verify_sec);
            output_string("\n");
        } else {
            output_string("Warning: Invalid time data returned, skipping write test\n");
        }

        release_port(rtc->control_port);
        release_port(rtc->data_port);
        free(rtc);
        output_string("RTC cleanup completed\n");
    } else {
        output_string("RTC initialization failed\n");
    }
}

TEST(rtc_interrupt_registration) {
    output_string("Testing RTC interrupt registration...\n");
    
    RTCDriver* rtc = init_rtc();
    ASSERT(rtc != NULL, "RTC initialization should succeed for interrupt test");
    
    if (rtc != NULL) {
        int result = enable_rtc_interrupts(rtc, rtc_interrupt_handler);
        if (result == 0) {
            output_string("RTC interrupts enabled successfully\n");

            output_string("RTC interrupt handler registered successfully\n");

            disable_rtc_interrupts(rtc);
        } else {
            output_string("RTC interrupt enable failed\n");
            ASSERT(false, "RTC interrupt registration should succeed");
        }
        
        release_port(rtc->control_port);
        release_port(rtc->data_port);
        free(rtc);
    } else {
        ASSERT(false, "RTC initialization failed for interrupt test");
    }
}

void run_memory_tests() {
    test_entry_t memory_tests[] = {
        TEST_ENTRY(memory_alloc_basic),
        TEST_ENTRY(memory_alloc_zero),
        TEST_ENTRY(memory_alloc_and_free),
        TEST_ENTRY(memory_multiple_alloc_free)
    };
    
    run_tests(memory_tests, sizeof(memory_tests) / sizeof(memory_tests[0]));
}

void run_rtc_tests() {
    test_entry_t rtc_tests[] = {
        TEST_ENTRY(rtc_basic_init),
        TEST_ENTRY(rtc_interrupt_registration)
    };
    
    run_tests(rtc_tests, sizeof(rtc_tests) / sizeof(rtc_tests[0]));
}