#include "test.h"
#include "terminal.h"
#include "io.h"
#include "memory.h"

#include <stdint.h>

static int tests_passed = 0;
static int tests_failed = 0;

void test_assert(int condition, const char* message, const char* file, int line) {
    if (condition) {
        output_string("PASS: ");
        output_string(message);
        output_string(" (");
        output_string(file);
        output_string(":");
        put_i32(line);
        output_string(")\n");
        tests_passed++;
    } else {
        output_string("FAIL: ");
        output_string(message);
        output_string(" (");
        output_string(file);
        output_string(":");
        put_i32(line);
        output_string(")\n");
        tests_failed++;
    }
}

void test_assert_equal(int expected, int actual, const char* message, const char* file, int line) {
    if (expected == actual) {
        output_string("PASS: ");
        output_string(message);
        output_string(" (");
        output_string(file);
        output_string(":");
        put_i32(line);
        output_string(") Expected: ");
        put_i32(expected);
        output_string(", Actual: ");
        put_i32(actual);
        output_string("\n");
        tests_passed++;
    } else {
        output_string("FAIL: ");
        output_string(message);
        output_string(" (");
        output_string(file);
        output_string(":");
        put_i32(line);
        output_string(") Expected: ");
        put_i32(expected);
        output_string(", Actual: ");
        put_i32(actual);
        output_string("\n");
        tests_failed++;
    }
}

void run_tests(test_entry_t* tests, int num_tests) {
    tests_passed = 0;
    tests_failed = 0;
    
    output_string("Running ");
    put_i32(num_tests);
    output_string(" tests...\n\n");
    
    for (int i = 0; i < num_tests; i++) {
        output_string("Running test: ");
        output_string(tests[i].name);
        output_string("\n");
        
        tests[i].func();
    }
    
    output_string("\nTest Results: ");
    put_i32(tests_passed);
    output_string(" passed, ");
    put_i32(tests_failed);
    output_string(" failed\n");
    
    if (tests_failed == 0) {
        output_string("All tests passed!\n");
    } else {
        output_string("Some tests failed!\n");
    }
}

void exit_after_all_tests(int exit_code) {
    exit_qemu(exit_code);
}