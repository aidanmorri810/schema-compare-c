#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Test statistics */
typedef struct {
    int total;
    int passed;
    int failed;
    int skipped;
} TestStats;

/* Global test statistics */
extern TestStats g_test_stats;

/* Color codes for output */
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_BOLD    "\033[1m"

/* Test result macros */
#define TEST_PASS() do { \
    g_test_stats.passed++; \
    g_test_stats.total++; \
    printf(COLOR_GREEN "✓ PASS" COLOR_RESET "\n"); \
    return true; \
} while(0)

#define TEST_FAIL(msg, ...) do { \
    g_test_stats.failed++; \
    g_test_stats.total++; \
    printf(COLOR_RED "✗ FAIL" COLOR_RESET " - "); \
    printf(msg, ##__VA_ARGS__); \
    printf("\n"); \
    return false; \
} while(0)

#define TEST_SKIP(msg, ...) do { \
    g_test_stats.skipped++; \
    g_test_stats.total++; \
    printf(COLOR_YELLOW "⊘ SKIP" COLOR_RESET " - "); \
    printf(msg, ##__VA_ARGS__); \
    printf("\n"); \
    return false; \
} while(0)

/* Assertion macros */
#define ASSERT_TRUE(expr) do { \
    if (!(expr)) { \
        TEST_FAIL("Assertion failed: %s (at %s:%d)", #expr, __FILE__, __LINE__); \
    } \
} while(0)

#define ASSERT_FALSE(expr) do { \
    if (expr) { \
        TEST_FAIL("Assertion failed: !(%s) (at %s:%d)", #expr, __FILE__, __LINE__); \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        TEST_FAIL("Assertion failed: %s == %s (%ld != %ld) (at %s:%d)", \
                  #a, #b, (long)(a), (long)(b), __FILE__, __LINE__); \
    } \
} while(0)

#define ASSERT_NEQ(a, b) do { \
    if ((a) == (b)) { \
        TEST_FAIL("Assertion failed: %s != %s (%ld == %ld) (at %s:%d)", \
                  #a, #b, (long)(a), (long)(b), __FILE__, __LINE__); \
    } \
} while(0)

#define ASSERT_STR_EQ(a, b) do { \
    if (strcmp((a), (b)) != 0) { \
        TEST_FAIL("Assertion failed: %s == %s (\"%s\" != \"%s\") (at %s:%d)", \
                  #a, #b, (a), (b), __FILE__, __LINE__); \
    } \
} while(0)

#define ASSERT_STR_NEQ(a, b) do { \
    if (strcmp((a), (b)) == 0) { \
        TEST_FAIL("Assertion failed: %s != %s (\"%s\" == \"%s\") (at %s:%d)", \
                  #a, #b, (a), (b), __FILE__, __LINE__); \
    } \
} while(0)

#define ASSERT_NULL(ptr) do { \
    if ((ptr) != NULL) { \
        TEST_FAIL("Assertion failed: %s == NULL (ptr is not NULL) (at %s:%d)", \
                  #ptr, __FILE__, __LINE__); \
    } \
} while(0)

#define ASSERT_NOT_NULL(ptr) do { \
    if ((ptr) == NULL) { \
        TEST_FAIL("Assertion failed: %s != NULL (ptr is NULL) (at %s:%d)", \
                  #ptr, __FILE__, __LINE__); \
    } \
} while(0)

#define ASSERT_PTR_EQ(a, b) do { \
    if ((a) != (b)) { \
        TEST_FAIL("Assertion failed: %s == %s (%p != %p) (at %s:%d)", \
                  #a, #b, (void*)(a), (void*)(b), __FILE__, __LINE__); \
    } \
} while(0)

#define ASSERT_FLOAT_EQ(a, b, epsilon) do { \
    double diff = (a) - (b); \
    if (diff < 0) diff = -diff; \
    if (diff > (epsilon)) { \
        TEST_FAIL("Assertion failed: %s ≈ %s (%.10f != %.10f, diff=%.10f > %.10f) (at %s:%d)", \
                  #a, #b, (double)(a), (double)(b), diff, (double)(epsilon), __FILE__, __LINE__); \
    } \
} while(0)

/* Test function typedef */
typedef bool (*TestFunc)(void);

/* Test case structure */
typedef struct {
    const char *name;
    TestFunc func;
    const char *suite;
} TestCase;

/* Test suite structure */
typedef struct {
    const char *name;
    void (*setup)(void);
    void (*teardown)(void);
} TestSuite;

/* Test registration macros */
#define TEST_CASE(suite_name, test_name) \
    bool test_##suite_name##_##test_name(void)

#define RUN_TEST(suite_name, test_name) \
    run_test(#suite_name, #test_name, test_##suite_name##_##test_name)

/* Test runner functions */
void test_init(void);
void test_summary(void);
bool run_test(const char *suite, const char *name, TestFunc func);
void run_test_suite(const char *suite_name, void (*setup)(void),
                    void (*teardown)(void), TestCase *tests, int num_tests);

/* Helper to print test header */
static inline void test_header(const char *suite, const char *name) {
    printf(COLOR_CYAN "[ RUN      ]" COLOR_RESET " %s::%s ", suite, name);
    fflush(stdout);
}

/* Helper to check if tests should run for a specific suite */
extern const char *g_test_filter_suite;
extern const char *g_test_filter_name;

static inline bool should_run_test(const char *suite, const char *name) {
    if (g_test_filter_suite && strcmp(g_test_filter_suite, suite) != 0) {
        return false;
    }
    if (g_test_filter_name && strcmp(g_test_filter_name, name) != 0) {
        return false;
    }
    return true;
}

#endif /* TEST_FRAMEWORK_H */
