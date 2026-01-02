#include "../test_framework.h"
#include "report.h"
#include "diff.h"
#include <string.h>

/* Test: Report options default */
TEST_CASE(report, report_options_default) {
    ReportOptions *opts = report_options_default();
    ASSERT_NOT_NULL(opts);

    /* Check default values exist */
    ASSERT_TRUE(opts->format == REPORT_FORMAT_TEXT ||
                opts->format == REPORT_FORMAT_MARKDOWN ||
                opts->format == REPORT_FORMAT_JSON);

    report_options_free(opts);
    TEST_PASS();
}

/* Test: Severity icon */
TEST_CASE(report, severity_icon) {
    const char *icon;

    icon = severity_icon(SEVERITY_CRITICAL);
    ASSERT_NOT_NULL(icon);
    ASSERT_TRUE(strlen(icon) > 0);

    icon = severity_icon(SEVERITY_WARNING);
    ASSERT_NOT_NULL(icon);

    icon = severity_icon(SEVERITY_INFO);
    ASSERT_NOT_NULL(icon);

    TEST_PASS();
}

/* Test: Severity color */
TEST_CASE(report, severity_color) {
    const char *color;

    color = severity_color_start(SEVERITY_CRITICAL);
    ASSERT_NOT_NULL(color);

    color = severity_color_start(SEVERITY_WARNING);
    ASSERT_NOT_NULL(color);

    color = severity_color_start(SEVERITY_INFO);
    ASSERT_NOT_NULL(color);

    color = severity_color_end();
    ASSERT_NOT_NULL(color);

    TEST_PASS();
}

/* Test: Diff prefix */
TEST_CASE(report, diff_prefix) {
    const char *prefix;

    prefix = diff_prefix(true, false);  /* Added */
    ASSERT_NOT_NULL(prefix);

    prefix = diff_prefix(false, true);  /* Removed */
    ASSERT_NOT_NULL(prefix);

    prefix = diff_prefix(false, false); /* Modified */
    ASSERT_NOT_NULL(prefix);

    TEST_PASS();
}

/* Test: Generate summary for empty diff */
TEST_CASE(report, generate_summary_empty) {
    SchemaDiff *diff = schema_diff_create("public");
    ASSERT_NOT_NULL(diff);

    ReportOptions *opts = report_options_default();
    char *summary = generate_summary(diff, opts);
    ASSERT_NOT_NULL(summary);

    /* Summary should mention no changes or similar */
    ASSERT_TRUE(strlen(summary) > 0);

    free(summary);
    report_options_free(opts);
    schema_diff_free(diff);
    TEST_PASS();
}

/* Test: Generate report for empty diff */
TEST_CASE(report, generate_report_empty) {
    SchemaDiff *diff = schema_diff_create("public");
    ASSERT_NOT_NULL(diff);

    ReportOptions *opts = report_options_default();
    char *report = generate_report(diff, opts);
    ASSERT_NOT_NULL(report);
    ASSERT_TRUE(strlen(report) > 0);

    free(report);
    report_options_free(opts);
    schema_diff_free(diff);
    TEST_PASS();
}

/* Test: Generate table diff report */
TEST_CASE(report, generate_table_diff_report) {
    TableDiff *td = table_diff_create("users");
    ASSERT_NOT_NULL(td);

    td->table_added = true;

    ReportOptions *opts = report_options_default();
    char *report = generate_table_diff_report(td, opts);
    ASSERT_NOT_NULL(report);

    /* Report should mention the table name */
    ASSERT_TRUE(strstr(report, "users") != NULL);

    free(report);
    report_options_free(opts);
    table_diff_free(td);
    TEST_PASS();
}

/* Test: Text format report */
TEST_CASE(report, text_format_report) {
    SchemaDiff *diff = schema_diff_create("public");
    ASSERT_NOT_NULL(diff);

    ReportOptions *opts = report_options_default();
    opts->format = REPORT_FORMAT_TEXT;

    char *report = generate_report(diff, opts);
    ASSERT_NOT_NULL(report);

    free(report);
    report_options_free(opts);
    schema_diff_free(diff);
    TEST_PASS();
}

/* Test: Markdown format report */
TEST_CASE(report, markdown_format_report) {
    SchemaDiff *diff = schema_diff_create("public");
    ASSERT_NOT_NULL(diff);

    ReportOptions *opts = report_options_default();
    opts->format = REPORT_FORMAT_MARKDOWN;

    char *report = generate_report(diff, opts);
    ASSERT_NOT_NULL(report);

    free(report);
    report_options_free(opts);
    schema_diff_free(diff);
    TEST_PASS();
}

/* Test suite definition */
static TestCase report_tests[] = {
    {"report_options_default", test_report_report_options_default, "report"},
    {"severity_icon", test_report_severity_icon, "report"},
    {"severity_color", test_report_severity_color, "report"},
    {"diff_prefix", test_report_diff_prefix, "report"},
    {"generate_summary_empty", test_report_generate_summary_empty, "report"},
    {"generate_report_empty", test_report_generate_report_empty, "report"},
    {"generate_table_diff_report", test_report_generate_table_diff_report, "report"},
    {"text_format_report", test_report_text_format_report, "report"},
    {"markdown_format_report", test_report_markdown_format_report, "report"},
};

void run_report_tests(void) {
    run_test_suite("report", NULL, NULL, report_tests,
                   sizeof(report_tests) / sizeof(report_tests[0]));
}
