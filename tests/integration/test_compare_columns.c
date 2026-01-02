#include "../test_framework.h"
#include "compare.h"
#include "parser.h"
#include "pg_create_table.h"
#include "diff.h"
#include "sc_memory.h"
#include "utils.h"
#include <string.h>

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/* Reusable helper from test_compare_integration.c */
static CreateTableStmt *parse_table_from_file(const char *filename) {
    char *sql = read_file_to_string(filename);
    if (!sql) return NULL;
    Parser *parser = parser_create(sql);
    if (!parser) {
        free(sql);
        return NULL;
    }
    CreateTableStmt *stmt = parser_parse_create_table(parser);
    parser_destroy(parser);
    free(sql);
    return stmt;
}

/* Helper to find column diff by name */
static ColumnDiff *find_column_diff(ColumnDiff *list, const char *column_name) {
    while (list) {
        if (strcmp(list->column_name, column_name) == 0) return list;
        list = list->next;
    }
    return NULL;
}

/* ============================================================================
 * Column-Level Tests (7 tests)
 * ============================================================================ */

/* Test 1: Column Addition */
TEST_CASE(compare_columns, column_add_single) {
    MemoryContext *ctx = memory_context_create("test_column_add_single");
    ASSERT_NOT_NULL(ctx);

    CreateTableStmt *source = parse_table_from_file("tests/data/compare_tests/baseline/users_base.sql");
    CreateTableStmt *target = parse_table_from_file("tests/data/compare_tests/column_changes/users_add_email.sql");
    ASSERT_NOT_NULL(source);
    ASSERT_NOT_NULL(target);

    CompareOptions *opts = compare_options_default();
    TableDiff *diff = compare_tables(source, target, opts, ctx);

    /* Validate: One column added */
    ASSERT_NOT_NULL(diff);
    ASSERT_EQ(diff->column_add_count, 1);
    ASSERT_EQ(diff->column_remove_count, 0);
    ASSERT_EQ(diff->column_modify_count, 0);

    /* Validate added column details */
    ColumnDiff *added = diff->columns_added;
    ASSERT_NOT_NULL(added);
    ASSERT_STR_EQ(added->column_name, "email");
    /* Type includes precision: VARCHAR(100) */
    ASSERT_TRUE(strstr(added->new_type, "varchar") != NULL || strstr(added->new_type, "VARCHAR") != NULL);

    compare_options_free(opts);
    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test 2: Column Removal */
TEST_CASE(compare_columns, column_remove_single) {
    MemoryContext *ctx = memory_context_create("test_column_remove_single");
    ASSERT_NOT_NULL(ctx);

    CreateTableStmt *source = parse_table_from_file("tests/data/compare_tests/baseline/users_base.sql");
    CreateTableStmt *target = parse_table_from_file("tests/data/compare_tests/column_changes/users_remove_age.sql");
    ASSERT_NOT_NULL(source);
    ASSERT_NOT_NULL(target);

    CompareOptions *opts = compare_options_default();
    TableDiff *diff = compare_tables(source, target, opts, ctx);

    /* Validate: One column removed */
    ASSERT_NOT_NULL(diff);
    ASSERT_EQ(diff->column_add_count, 0);
    ASSERT_EQ(diff->column_remove_count, 1);
    ASSERT_EQ(diff->column_modify_count, 0);

    /* Validate removed column details */
    ColumnDiff *removed = diff->columns_removed;
    ASSERT_NOT_NULL(removed);
    ASSERT_STR_EQ(removed->column_name, "age");

    compare_options_free(opts);
    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test 3: Column Type Change */
TEST_CASE(compare_columns, column_type_change) {
    MemoryContext *ctx = memory_context_create("test_column_type_change");
    ASSERT_NOT_NULL(ctx);

    CreateTableStmt *source = parse_table_from_file("tests/data/compare_tests/baseline/users_base.sql");
    CreateTableStmt *target = parse_table_from_file("tests/data/compare_tests/column_changes/users_type_change.sql");
    ASSERT_NOT_NULL(source);
    ASSERT_NOT_NULL(target);

    CompareOptions *opts = compare_options_default();
    TableDiff *diff = compare_tables(source, target, opts, ctx);

    /* Validate: Type changed on username column */
    ASSERT_NOT_NULL(diff);
    ASSERT_EQ(diff->column_modify_count, 1);

    /* Find the modified column */
    ColumnDiff *modified = find_column_diff(diff->columns_modified, "username");
    ASSERT_NOT_NULL(modified);
    ASSERT_TRUE(modified->type_changed);
    /* Types include precision */
    ASSERT_TRUE(strstr(modified->old_type, "varchar") != NULL || strstr(modified->old_type, "VARCHAR") != NULL);
    ASSERT_TRUE(strstr(modified->new_type, "text") != NULL || strstr(modified->new_type, "TEXT") != NULL);

    compare_options_free(opts);
    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test 4: Column Nullable Change (Add NOT NULL) */
TEST_CASE(compare_columns, column_add_not_null) {
    MemoryContext *ctx = memory_context_create("test_column_add_not_null");
    ASSERT_NOT_NULL(ctx);

    CreateTableStmt *source = parse_table_from_file("tests/data/compare_tests/baseline/users_base.sql");
    CreateTableStmt *target = parse_table_from_file("tests/data/compare_tests/column_changes/users_add_not_null.sql");
    ASSERT_NOT_NULL(source);
    ASSERT_NOT_NULL(target);

    CompareOptions *opts = compare_options_default();
    TableDiff *diff = compare_tables(source, target, opts, ctx);

    /* Validate: Nullable changed on age column */
    ASSERT_NOT_NULL(diff);
    ASSERT_EQ(diff->column_modify_count, 1);

    /* Find the modified column */
    ColumnDiff *modified = find_column_diff(diff->columns_modified, "age");
    ASSERT_NOT_NULL(modified);
    ASSERT_TRUE(modified->nullable_changed);

    compare_options_free(opts);
    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test 5: Add Default Value */
TEST_CASE(compare_columns, column_add_default) {
    MemoryContext *ctx = memory_context_create("test_column_add_default");
    ASSERT_NOT_NULL(ctx);

    CreateTableStmt *source = parse_table_from_file("tests/data/compare_tests/baseline/users_base.sql");
    CreateTableStmt *target = parse_table_from_file("tests/data/compare_tests/column_changes/users_add_default.sql");
    ASSERT_NOT_NULL(source);
    ASSERT_NOT_NULL(target);

    CompareOptions *opts = compare_options_default();
    TableDiff *diff = compare_tables(source, target, opts, ctx);

    /* Validate: Default changed on age column */
    ASSERT_NOT_NULL(diff);
    ASSERT_EQ(diff->column_modify_count, 1);

    /* Find the modified column */
    ColumnDiff *modified = find_column_diff(diff->columns_modified, "age");
    ASSERT_NOT_NULL(modified);
    ASSERT_TRUE(modified->default_changed);
    ASSERT_NOT_NULL(modified->new_default);

    compare_options_free(opts);
    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test 6: Change Default Value */
TEST_CASE(compare_columns, column_change_default) {
    MemoryContext *ctx = memory_context_create("test_column_change_default");
    ASSERT_NOT_NULL(ctx);

    CreateTableStmt *source = parse_table_from_file("tests/data/compare_tests/baseline/users_base.sql");
    CreateTableStmt *target = parse_table_from_file("tests/data/compare_tests/column_changes/users_change_default.sql");
    ASSERT_NOT_NULL(source);
    ASSERT_NOT_NULL(target);

    CompareOptions *opts = compare_options_default();
    TableDiff *diff = compare_tables(source, target, opts, ctx);

    /* Validate: Default changed on created_at column */
    ASSERT_NOT_NULL(diff);
    ASSERT_EQ(diff->column_modify_count, 1);

    /* Find the modified column */
    ColumnDiff *modified = find_column_diff(diff->columns_modified, "created_at");
    ASSERT_NOT_NULL(modified);
    ASSERT_TRUE(modified->default_changed);
    ASSERT_NOT_NULL(modified->old_default);
    ASSERT_NOT_NULL(modified->new_default);

    compare_options_free(opts);
    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test 7: Multiple Column Changes */
TEST_CASE(compare_columns, multiple_column_changes) {
    MemoryContext *ctx = memory_context_create("test_multiple_column_changes");
    ASSERT_NOT_NULL(ctx);

    CreateTableStmt *source = parse_table_from_file("tests/data/compare_tests/baseline/users_base.sql");
    CreateTableStmt *target = parse_table_from_file("tests/data/compare_tests/column_changes/users_multi_changes.sql");
    ASSERT_NOT_NULL(source);
    ASSERT_NOT_NULL(target);

    CompareOptions *opts = compare_options_default();
    TableDiff *diff = compare_tables(source, target, opts, ctx);

    /* Validate: email added, age removed */
    ASSERT_NOT_NULL(diff);
    ASSERT_EQ(diff->column_add_count, 1);
    ASSERT_EQ(diff->column_remove_count, 1);

    /* Validate added column */
    ColumnDiff *added = find_column_diff(diff->columns_added, "email");
    ASSERT_NOT_NULL(added);

    /* Validate removed column */
    ColumnDiff *removed = find_column_diff(diff->columns_removed, "age");
    ASSERT_NOT_NULL(removed);

    compare_options_free(opts);
    memory_context_destroy(ctx);
    TEST_PASS();
}

/* ============================================================================
 * Test Suite Definition
 * ============================================================================ */

static TestCase compare_columns_tests[] = {
    {"column_add_single", test_compare_columns_column_add_single, "compare_columns"},
    {"column_remove_single", test_compare_columns_column_remove_single, "compare_columns"},
    {"column_type_change", test_compare_columns_column_type_change, "compare_columns"},
    {"column_add_not_null", test_compare_columns_column_add_not_null, "compare_columns"},
    {"column_add_default", test_compare_columns_column_add_default, "compare_columns"},
    {"column_change_default", test_compare_columns_column_change_default, "compare_columns"},
    {"multiple_column_changes", test_compare_columns_multiple_column_changes, "compare_columns"},
};

void run_compare_columns_tests(void) {
    run_test_suite("compare_columns", NULL, NULL, compare_columns_tests,
                   sizeof(compare_columns_tests) / sizeof(compare_columns_tests[0]));
}
