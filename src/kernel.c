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

void run_rtc_tests();  

void kernel_main(uint32_t magic, uint32_t multiboot_info_ptr) {
    init_output();
    
    output_string("hi shogun from c - Test Mode\n");

    output_string("Initializing memory allocator for tests...\n");
    init_allocator(multiboot_info_ptr);
    
    output_string("Initializing port manager...\n");
    init_port_manager();
    
    run_memory_tests();
    
    output_string("\nRunning RTC tests...\n");
    run_rtc_tests();

    exit_after_all_tests(0);
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
        TEST_ENTRY(rtc_basic_init)
    };
    
    run_tests(rtc_tests, sizeof(rtc_tests) / sizeof(rtc_tests[0]));
}