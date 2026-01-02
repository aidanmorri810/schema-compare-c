#include "../test_framework.h"
#include "diff.h"
#include <string.h>

/* Test: Create diff */
TEST_CASE(diff, create_diff) {
    Diff *diff = diff_create(DIFF_COLUMN_ADDED, SEVERITY_WARNING, "users", "email");
    ASSERT_NOT_NULL(diff);
    ASSERT_EQ(diff->type, DIFF_COLUMN_ADDED);
    ASSERT_EQ(diff->severity, SEVERITY_WARNING);
    ASSERT_NOT_NULL(diff->table_name);
    ASSERT_NOT_NULL(diff->element_name);
    ASSERT_NULL(diff->next);

    diff_free(diff);
    TEST_PASS();
}

/* Test: Diff severity - critical */
TEST_CASE(diff, diff_severity_critical) {
    DiffSeverity severity;

    severity = diff_determine_severity(DIFF_TABLE_REMOVED);
    ASSERT_EQ(severity, SEVERITY_CRITICAL);

    severity = diff_determine_severity(DIFF_COLUMN_REMOVED);
    ASSERT_EQ(severity, SEVERITY_CRITICAL);

    severity = diff_determine_severity(DIFF_COLUMN_TYPE_CHANGED);
    ASSERT_EQ(severity, SEVERITY_CRITICAL);

    TEST_PASS();
}

/* Test: Diff severity - warning */
TEST_CASE(diff, diff_severity_warning) {
    DiffSeverity severity;

    severity = diff_determine_severity(DIFF_TABLE_ADDED);
    ASSERT_EQ(severity, SEVERITY_WARNING);

    severity = diff_determine_severity(DIFF_COLUMN_ADDED);
    ASSERT_EQ(severity, SEVERITY_WARNING);

    severity = diff_determine_severity(DIFF_COLUMN_NULLABLE_CHANGED);
    ASSERT_EQ(severity, SEVERITY_WARNING);

    TEST_PASS();
}

/* Test: Diff severity - info */
TEST_CASE(diff, diff_severity_info) {
    DiffSeverity severity;

    severity = diff_determine_severity(DIFF_COLUMN_DEFAULT_CHANGED);
    ASSERT_EQ(severity, SEVERITY_INFO);

    severity = diff_determine_severity(DIFF_CONSTRAINT_ADDED);
    ASSERT_EQ(severity, SEVERITY_INFO);

    TEST_PASS();
}

/* Test: Diff type to string */
TEST_CASE(diff, diff_type_to_string) {
    const char *str;

    str = diff_type_to_string(DIFF_TABLE_ADDED);
    ASSERT_NOT_NULL(str);
    ASSERT_TRUE(strlen(str) > 0);

    str = diff_type_to_string(DIFF_COLUMN_TYPE_CHANGED);
    ASSERT_NOT_NULL(str);
    ASSERT_TRUE(strlen(str) > 0);

    TEST_PASS();
}

/* Test: Diff severity to string */
TEST_CASE(diff, diff_severity_to_string) {
    const char *str;

    str = diff_severity_to_string(SEVERITY_CRITICAL);
    ASSERT_NOT_NULL(str);
    ASSERT_TRUE(strlen(str) > 0);

    str = diff_severity_to_string(SEVERITY_WARNING);
    ASSERT_NOT_NULL(str);

    str = diff_severity_to_string(SEVERITY_INFO);
    ASSERT_NOT_NULL(str);

    TEST_PASS();
}

/* Test: Diff list append */
TEST_CASE(diff, diff_list_append) {
    Diff *list = NULL;

    Diff *diff1 = diff_create(DIFF_COLUMN_ADDED, SEVERITY_INFO, "users", "email");
    diff_append(&list, diff1);
    ASSERT_NOT_NULL(list);
    ASSERT_EQ(list, diff1);

    Diff *diff2 = diff_create(DIFF_COLUMN_REMOVED, SEVERITY_WARNING, "users", "old_field");
    diff_append(&list, diff2);
    ASSERT_NOT_NULL(list->next);
    ASSERT_EQ(list->next, diff2);

    diff_list_free(list);
    TEST_PASS();
}

/* Test: Diff set values */
TEST_CASE(diff, diff_set_values) {
    Diff *diff = diff_create(DIFF_COLUMN_TYPE_CHANGED, SEVERITY_WARNING, "users", "id");

    diff_set_values(diff, "INTEGER", "BIGINT");
    ASSERT_NOT_NULL(diff->old_value);
    ASSERT_NOT_NULL(diff->new_value);
    ASSERT_STR_EQ(diff->old_value, "INTEGER");
    ASSERT_STR_EQ(diff->new_value, "BIGINT");

    diff_free(diff);
    TEST_PASS();
}

/* Test: Diff set description */
TEST_CASE(diff, diff_set_description) {
    Diff *diff = diff_create(DIFF_COLUMN_ADDED, SEVERITY_INFO, "users", "email");

    diff_set_description(diff, "Added email column");
    ASSERT_NOT_NULL(diff->description);
    ASSERT_STR_EQ(diff->description, "Added email column");

    diff_free(diff);
    TEST_PASS();
}

/* Test: SchemaDiff creation */
TEST_CASE(diff, schema_diff_create) {
    SchemaDiff *sd = schema_diff_create("public");
    ASSERT_NOT_NULL(sd);
    ASSERT_NOT_NULL(sd->schema_name);
    ASSERT_STR_EQ(sd->schema_name, "public");
    ASSERT_EQ(sd->total_diffs, 0);
    ASSERT_EQ(sd->tables_added, 0);
    ASSERT_EQ(sd->tables_removed, 0);
    ASSERT_EQ(sd->tables_modified, 0);

    schema_diff_free(sd);
    TEST_PASS();
}

/* Test: TableDiff creation */
TEST_CASE(diff, table_diff_create) {
    TableDiff *td = table_diff_create("users");
    ASSERT_NOT_NULL(td);
    ASSERT_NOT_NULL(td->table_name);
    ASSERT_STR_EQ(td->table_name, "users");
    ASSERT_FALSE(td->table_added);
    ASSERT_FALSE(td->table_removed);
    ASSERT_FALSE(td->table_modified);

    table_diff_free(td);
    TEST_PASS();
}

/* Test: ColumnDiff creation */
TEST_CASE(diff, column_diff_create) {
    ColumnDiff *cd = column_diff_create("email");
    ASSERT_NOT_NULL(cd);
    ASSERT_NOT_NULL(cd->column_name);
    ASSERT_STR_EQ(cd->column_name, "email");
    ASSERT_NULL(cd->next);

    column_diff_free(cd);
    TEST_PASS();
}

/* Test: ConstraintDiff creation */
TEST_CASE(diff, constraint_diff_create) {
    ConstraintDiff *cd = constraint_diff_create("pk_users");
    ASSERT_NOT_NULL(cd);
    ASSERT_NOT_NULL(cd->constraint_name);
    ASSERT_STR_EQ(cd->constraint_name, "pk_users");
    ASSERT_NULL(cd->next);

    constraint_diff_free(cd);
    TEST_PASS();
}

/* Test suite definition */
static TestCase diff_tests[] = {
    {"create_diff", test_diff_create_diff, "diff"},
    {"diff_severity_critical", test_diff_diff_severity_critical, "diff"},
    {"diff_severity_warning", test_diff_diff_severity_warning, "diff"},
    {"diff_severity_info", test_diff_diff_severity_info, "diff"},
    {"diff_type_to_string", test_diff_diff_type_to_string, "diff"},
    {"diff_severity_to_string", test_diff_diff_severity_to_string, "diff"},
    {"diff_list_append", test_diff_diff_list_append, "diff"},
    {"diff_set_values", test_diff_diff_set_values, "diff"},
    {"diff_set_description", test_diff_diff_set_description, "diff"},
    {"schema_diff_create", test_diff_schema_diff_create, "diff"},
    {"table_diff_create", test_diff_table_diff_create, "diff"},
    {"column_diff_create", test_diff_column_diff_create, "diff"},
    {"constraint_diff_create", test_diff_constraint_diff_create, "diff"},
};

void run_diff_tests(void) {
    run_test_suite("diff", NULL, NULL, diff_tests,
                   sizeof(diff_tests) / sizeof(diff_tests[0]));
}
