#include <stdint.h>
#include <stddef.h>
#include "terminal.h"
#include "libc.h"
#include "multiboot.h"
#include "memory.h"
#include "io.h"
#include "test.h"

void kernel_main(uint32_t magic, uint32_t multiboot_info_ptr) {
    init_output();
    
    output_string("hi shogun from c - Test Mode\n");

    output_string("Initializing memory allocator for tests...\n");
    init_allocator(multiboot_info_ptr);
    
    run_memory_tests();
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

void run_memory_tests() {
    test_entry_t memory_tests[] = {
        TEST_ENTRY(memory_alloc_basic),
        TEST_ENTRY(memory_alloc_zero),
        TEST_ENTRY(memory_alloc_and_free),
        TEST_ENTRY(memory_multiple_alloc_free)
    };
    
    run_tests(memory_tests, sizeof(memory_tests) / sizeof(memory_tests[0]));
}