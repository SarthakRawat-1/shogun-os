#ifndef TEST_H
#define TEST_H

#include <stdint.h>

typedef void (*test_func_t)(void);

typedef struct {
    const char* name;
    test_func_t func;
} test_entry_t;

void run_tests(test_entry_t* tests, int num_tests);

void test_assert(int condition, const char* message, const char* file, int line);
void test_assert_equal(int expected, int actual, const char* message, const char* file, int line);

#define ASSERT(condition, message) test_assert(condition, message, __FILE__, __LINE__)
#define ASSERT_EQUAL(expected, actual, message) test_assert_equal(expected, actual, message, __FILE__, __LINE__)

#define TEST(name) void test_##name(void)

#define CREATE_TEST(name) \
    void test_##name(void); \
    static test_entry_t test_entry_##name = {#name, test_##name}; \
    void test_##name(void)

#define TEST_ENTRY(name) {#name, test_##name}

void run_memory_tests();
void exit_after_all_tests(int exit_code);

#endif