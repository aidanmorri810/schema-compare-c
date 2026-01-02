#include "test_framework.h"

/* Global test statistics */
TestStats g_test_stats = {0, 0, 0, 0};

/* Global test filters */
const char *g_test_filter_suite = NULL;
const char *g_test_filter_name = NULL;

/* Initialize test framework */
void test_init(void) {
    g_test_stats.total = 0;
    g_test_stats.passed = 0;
    g_test_stats.failed = 0;
    g_test_stats.skipped = 0;
}

/* Print test summary */
void test_summary(void) {
    printf("\n");
    printf(COLOR_BOLD "========================================\n" COLOR_RESET);
    printf(COLOR_BOLD "Test Summary\n" COLOR_RESET);
    printf(COLOR_BOLD "========================================\n" COLOR_RESET);
    printf("Total:   %d\n", g_test_stats.total);
    printf(COLOR_GREEN "Passed:  %d\n" COLOR_RESET, g_test_stats.passed);
    printf(COLOR_RED "Failed:  %d\n" COLOR_RESET, g_test_stats.failed);
    printf(COLOR_YELLOW "Skipped: %d\n" COLOR_RESET, g_test_stats.skipped);
    printf(COLOR_BOLD "========================================\n" COLOR_RESET);

    if (g_test_stats.failed == 0 && g_test_stats.total > 0) {
        printf(COLOR_GREEN COLOR_BOLD "All tests passed!\n" COLOR_RESET);
    } else if (g_test_stats.failed > 0) {
        printf(COLOR_RED COLOR_BOLD "%d test(s) failed!\n" COLOR_RESET, g_test_stats.failed);
    }
}

/* Run a single test */
bool run_test(const char *suite, const char *name, TestFunc func) {
    if (!should_run_test(suite, name)) {
        return true; /* Skip silently */
    }

    test_header(suite, name);
    bool result = func();
    return result;
}

/* Run a test suite */
void run_test_suite(const char *suite_name, void (*setup)(void),
                    void (*teardown)(void), TestCase *tests, int num_tests) {
    printf("\n");
    printf(COLOR_MAGENTA COLOR_BOLD "========================================\n" COLOR_RESET);
    printf(COLOR_MAGENTA COLOR_BOLD "Test Suite: %s\n" COLOR_RESET, suite_name);
    printf(COLOR_MAGENTA COLOR_BOLD "========================================\n" COLOR_RESET);

    for (int i = 0; i < num_tests; i++) {
        if (!should_run_test(suite_name, tests[i].name)) {
            continue;
        }

        if (setup) {
            setup();
        }

        test_header(suite_name, tests[i].name);
        tests[i].func();

        if (teardown) {
            teardown();
        }
    }
}
