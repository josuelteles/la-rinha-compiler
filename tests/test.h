/**
 * @file test.h
 * @brief This file contains the definitions and macros for running tests.
 *
 * @author Josuel Teles
 */

#ifndef _TEST_H
#define _TEST_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>

typedef struct {
    const char* name;
    void (*test_func)(void);
} _test_t;


#define TEST(name)                               \
    void name##_test_function(void);              \
    _test_t name##_test = {#name, name##_test_function}; \
    void name##_test_function(void)

#define _TEST_COLOR_GREEN   "\x1b[32;1m"
#define _TEST_COLOR_RED     "\x1b[31;1m"
#define _TEST_COLOR_RESET   "\x1b[0m"

#define EXPECT_TEMPLATE(actual, expected, condition, format, ...) \
   {                                                         \
        if (condition) {                                          \
            printf("  " _TEST_COLOR_GREEN "PASSED" _TEST_COLOR_RESET ": %s\n", #actual); \
        } else {                                                 \
            printf("  " _TEST_COLOR_RED "FAILED" _TEST_COLOR_RESET ": %s (" format ")\n", \
                   #actual, ##__VA_ARGS__);                      \
        }                                                        \
    }

#define EXPECT_EQ(actual, expected) \
    EXPECT_TEMPLATE(actual, expected, (actual) == (expected), "%d == %d", actual, expected)

#define EXPECT_NE(actual, expected) \
    EXPECT_TEMPLATE(actual, expected, (actual) != (expected), "%d != %d", actual, expected)

#define EXPECT_TRUE(condition) \
    EXPECT_TEMPLATE(condition, true, (condition), "true")

#define EXPECT_FALSE(condition) \
    EXPECT_TEMPLATE(condition, false, !(condition), "false")

#define EXPECT_STREQ(actual, expected) \
    EXPECT_TEMPLATE(actual, expected, strcmp(actual, expected) == 0, "'%s' == '%s'", actual, expected)

#define EXPECT_STRNEQ(actual, expected) \
    EXPECT_TEMPLATE(actual, expected, strcmp(actual, expected) != 0, "'%s' != '%s'", actual, expected)

bool run_tests(const _test_t tests[], int total) {
    int passed_count = 0;

    for (int i = 0; i < total; i++) {
        printf("Running rinha tests: %s\n", tests[i].name);
        fflush(stdout);

        tests[i].test_func();

        printf("\n");
        passed_count++;
    }

    printf("Tests passed: %d/%d\n", passed_count, total);

    return (passed_count == total);
}

#endif

