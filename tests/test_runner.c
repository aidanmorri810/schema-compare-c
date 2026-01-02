#include "test_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

/* Forward declarations of test suite runners */
void run_memory_tests(void);
void run_hash_table_tests(void);
void run_string_builder_tests(void);
void run_lexer_tests(void);
void run_parser_basic_tests(void);
void run_parser_columns_tests(void);
void run_parser_constraints_tests(void);
void run_parser_advanced_tests(void);
void run_compare_tests(void);
void run_diff_tests(void);
void run_report_tests(void);
void run_sql_generator_tests(void);
void run_parser_integration_tests(void);
void run_compare_integration_tests(void);
void run_workflow_integration_tests(void);
void run_db_reader_tests(void);
/* Nora tenant test suites */
void run_nora_tenant_test_table_tests(void);
void run_nora_tenant_claim_type_tests(void);
void run_nora_tenant_claim_status_tests(void);
void run_nora_tenant_claim_tests(void);
void run_nora_tenant_claim_document_tests(void);
void run_nora_tenant_claim_document_version_tests(void);
void run_nora_tenant_claim_document_tag_tests(void);
void run_nora_tenant_file_tests(void);
void run_nora_tenant_file_tag_tests(void);
void run_nora_tenant_note_tests(void);
void run_nora_tenant_note_tag_tests(void);
void run_nora_tenant_note_to_note_tag_tests(void);
void run_nora_tenant_note_document_tests(void);
void run_nora_tenant_review_tests(void);
void run_nora_tenant_review_iteration_tests(void);
void run_nora_tenant_review_iteration_document_tests(void);
void run_nora_tenant_feedback_tests(void);
void run_nora_tenant_feedback_type_tests(void);
void run_nora_tenant_role_tests(void);
void run_nora_tenant_user_role_tests(void);
void run_nora_tenant_tenant_user_data_tests(void);
void run_nora_tenant_dashboard_widget_tests(void);
void run_nora_tenant_failure_mode_tests(void);
/* Nora central test suites */
void run_nora_central_auth_user_tests(void);
void run_nora_central_auth_account_tests(void);
void run_nora_central_auth_session_tests(void);
void run_nora_central_auth_verification_tests(void);
void run_nora_central_tenant_tests(void);
void run_nora_central_user_tests(void);
void run_nora_central_tenant_user_tests(void);
/* Add more test suite declarations here */

/* Global filter variables (defined in test_framework.c) */
extern const char *g_test_filter_suite;
extern const char *g_test_filter_name;

static void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("\nOptions:\n");
    printf("  -h, --help              Show this help message\n");
    printf("  -v, --verbose           Verbose output\n");
    printf("  -s, --suite SUITE       Run only tests from SUITE\n");
    printf("  -t, --test TEST         Run only TEST (requires --suite)\n");
    printf("  -l, --list              List all test suites\n");
    printf("\nExamples:\n");
    printf("  %s                      # Run all tests\n", program_name);
    printf("  %s --suite memory       # Run only memory tests\n", program_name);
    printf("  %s --suite hash_table --test insert_and_get\n", program_name);
    printf("\n");
}

static void list_test_suites(void) {
    printf("Available test suites:\n");
    printf("  - memory\n");
    printf("  - hash_table\n");
    printf("  - string_builder\n");
    printf("  - lexer\n");
    printf("  - parser_basic\n");
    printf("  - parser_columns\n");
    printf("  - parser_constraints\n");
    printf("  - parser_advanced\n");
    printf("  - compare\n");
    printf("  - diff\n");
    printf("  - report\n");
    printf("  - sql_generator\n");
    printf("  - parser_integration\n");
    printf("  - compare_integration\n");
    printf("  - workflow_integration\n");
    printf("  - db_reader\n");
    printf("  - nora_tenant_test_table\n");
    printf("  - nora_tenant_claim_type\n");
    printf("  - nora_tenant_claim_status\n");
    printf("  - nora_tenant_claim\n");
    printf("  - nora_tenant_claim_document\n");
    printf("  - nora_tenant_claim_document_version\n");
    printf("  - nora_tenant_claim_document_tag\n");
    printf("  - nora_tenant_file\n");
    printf("  - nora_tenant_file_tag\n");
    printf("  - nora_tenant_note\n");
    printf("  - nora_tenant_note_tag\n");
    printf("  - nora_tenant_note_to_note_tag\n");
    printf("  - nora_tenant_note_document\n");
    printf("  - nora_tenant_review\n");
    printf("  - nora_tenant_review_iteration\n");
    printf("  - nora_tenant_review_iteration_document\n");
    printf("  - nora_tenant_feedback\n");
    printf("  - nora_tenant_feedback_type\n");
    printf("  - nora_tenant_role\n");
    printf("  - nora_tenant_user_role\n");
    printf("  - nora_tenant_tenant_user_data\n");
    printf("  - nora_tenant_dashboard_widget\n");
    printf("  - nora_tenant_failure_mode\n");
    printf("  - nora_central_auth_user\n");
    printf("  - nora_central_auth_account\n");
    printf("  - nora_central_auth_session\n");
    printf("  - nora_central_auth_verification\n");
    printf("  - nora_central_tenant\n");
    printf("  - nora_central_user\n");
    printf("  - nora_central_tenant_user\n");
}

int main(int argc, char **argv) {
    const char *suite_filter = NULL;
    const char *test_filter = NULL;
    bool list_only = false;

    /* Parse command-line arguments */
    static struct option long_options[] = {
        {"help",    no_argument,       0, 'h'},
        {"verbose", no_argument,       0, 'v'},
        {"suite",   required_argument, 0, 's'},
        {"test",    required_argument, 0, 't'},
        {"list",    no_argument,       0, 'l'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "hvs:t:l", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'h':
                print_usage(argv[0]);
                return 0;
            case 'v':
                /* Verbose mode - reserved for future use */
                break;
            case 's':
                suite_filter = optarg;
                break;
            case 't':
                test_filter = optarg;
                break;
            case 'l':
                list_only = true;
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    if (list_only) {
        list_test_suites();
        return 0;
    }

    /* Set global filters */
    g_test_filter_suite = suite_filter;
    g_test_filter_name = test_filter;

    /* Initialize test framework */
    test_init();

    /* Print header */
    printf("\n");
    printf(COLOR_BOLD COLOR_BLUE "========================================\n" COLOR_RESET);
    printf(COLOR_BOLD COLOR_BLUE "  Schema Compare Test Runner\n" COLOR_RESET);
    printf(COLOR_BOLD COLOR_BLUE "========================================\n" COLOR_RESET);

    if (suite_filter) {
        printf("Running suite: " COLOR_CYAN "%s" COLOR_RESET "\n", suite_filter);
    }
    if (test_filter) {
        printf("Running test: " COLOR_CYAN "%s" COLOR_RESET "\n", test_filter);
    }

    /* Run all test suites */
    run_memory_tests();
    run_hash_table_tests();
    run_string_builder_tests();
    run_lexer_tests();
    run_parser_basic_tests();
    run_parser_columns_tests();
    run_parser_constraints_tests();
    run_parser_advanced_tests();
    run_compare_tests();
    run_diff_tests();
    run_report_tests();
    run_sql_generator_tests();
    run_parser_integration_tests();
    run_compare_integration_tests();
    run_workflow_integration_tests();
    run_db_reader_tests();
    /* Nora tenant tests */
    run_nora_tenant_test_table_tests();
    run_nora_tenant_claim_type_tests();
    run_nora_tenant_claim_status_tests();
    run_nora_tenant_claim_tests();
    run_nora_tenant_claim_document_tests();
    run_nora_tenant_claim_document_version_tests();
    run_nora_tenant_claim_document_tag_tests();
    run_nora_tenant_file_tests();
    run_nora_tenant_file_tag_tests();
    run_nora_tenant_note_tests();
    run_nora_tenant_note_tag_tests();
    run_nora_tenant_note_to_note_tag_tests();
    run_nora_tenant_note_document_tests();
    run_nora_tenant_review_tests();
    run_nora_tenant_review_iteration_tests();
    run_nora_tenant_review_iteration_document_tests();
    run_nora_tenant_feedback_tests();
    run_nora_tenant_feedback_type_tests();
    run_nora_tenant_role_tests();
    run_nora_tenant_user_role_tests();
    run_nora_tenant_tenant_user_data_tests();
    run_nora_tenant_dashboard_widget_tests();
    run_nora_tenant_failure_mode_tests();
    /* Nora central tests */
    run_nora_central_auth_user_tests();
    run_nora_central_auth_account_tests();
    run_nora_central_auth_session_tests();
    run_nora_central_auth_verification_tests();
    run_nora_central_tenant_tests();
    run_nora_central_user_tests();
    run_nora_central_tenant_user_tests();
    /* Add more test suite calls here */

    /* Print summary */
    test_summary();

    /* Return exit code based on test results */
    return (g_test_stats.failed > 0) ? 1 : 0;
}
