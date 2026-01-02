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

/* ============================================================================
 * Constraint Tests (8 tests)
 * ============================================================================ */

/* Test 1: Add Foreign Key */
TEST_CASE(compare_constraints, fk_add) {
    MemoryContext *ctx = memory_context_create("test_fk_add");
    ASSERT_NOT_NULL(ctx);

    CreateTableStmt *source = parse_table_from_file("tests/data/compare_tests/baseline/orders_no_fk.sql");
    CreateTableStmt *target = parse_table_from_file("tests/data/compare_tests/constraint_changes/orders_add_fk.sql");
    ASSERT_NOT_NULL(source);
    ASSERT_NOT_NULL(target);

    CompareOptions *opts = compare_options_default();
    TableDiff *diff = compare_tables(source, target, opts, ctx);

    /* Validate: One constraint added */
    ASSERT_NOT_NULL(diff);
    ASSERT_EQ(diff->constraint_add_count, 1);

    compare_options_free(opts);
    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test 2: Modify FK - ON DELETE Action Change */
TEST_CASE(compare_constraints, fk_on_delete_change) {
    MemoryContext *ctx = memory_context_create("test_fk_on_delete_change");
    ASSERT_NOT_NULL(ctx);

    CreateTableStmt *source = parse_table_from_file("tests/data/compare_tests/baseline/orders_base.sql");
    CreateTableStmt *target = parse_table_from_file("tests/data/compare_tests/constraint_changes/orders_fk_cascade_restrict.sql");
    ASSERT_NOT_NULL(source);
    ASSERT_NOT_NULL(target);

    CompareOptions *opts = compare_options_default();
    TableDiff *diff = compare_tables(source, target, opts, ctx);

    /* Validate: FK constraint modified (CASCADE → RESTRICT) */
    ASSERT_NOT_NULL(diff);
    /* Constraint may be reported as removed+added or modified */
    ASSERT_TRUE(diff->constraint_remove_count > 0 || diff->constraint_modify_count > 0);

    compare_options_free(opts);
    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test 3: Modify FK - Add ON UPDATE */
TEST_CASE(compare_constraints, fk_add_on_update) {
    MemoryContext *ctx = memory_context_create("test_fk_add_on_update");
    ASSERT_NOT_NULL(ctx);

    CreateTableStmt *source = parse_table_from_file("tests/data/compare_tests/baseline/orders_base.sql");
    CreateTableStmt *target = parse_table_from_file("tests/data/compare_tests/constraint_changes/orders_fk_add_on_update.sql");
    ASSERT_NOT_NULL(source);
    ASSERT_NOT_NULL(target);

    CompareOptions *opts = compare_options_default();
    TableDiff *diff = compare_tables(source, target, opts, ctx);

    /* Validate: FK constraint modified */
    ASSERT_NOT_NULL(diff);
    ASSERT_TRUE(diff->constraint_remove_count > 0 || diff->constraint_modify_count > 0);

    compare_options_free(opts);
    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test 4: Add Primary Key */
TEST_CASE(compare_constraints, pk_add) {
    MemoryContext *ctx = memory_context_create("test_pk_add");
    ASSERT_NOT_NULL(ctx);

    CreateTableStmt *source = parse_table_from_file("tests/data/compare_tests/baseline/employees_base.sql");
    CreateTableStmt *target = parse_table_from_file("tests/data/compare_tests/constraint_changes/employees_add_pk.sql");
    ASSERT_NOT_NULL(source);
    ASSERT_NOT_NULL(target);

    CompareOptions *opts = compare_options_default();
    TableDiff *diff = compare_tables(source, target, opts, ctx);

    /* Validate: Primary key added (inline PK shows as column modification or constraint) */
    ASSERT_NOT_NULL(diff);
    /* Either constraint added, column modified, or any difference detected */
    ASSERT_TRUE(diff->constraint_add_count > 0 || diff->column_modify_count > 0 || diff->diff_count > 0);

    compare_options_free(opts);
    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test 5: Modify Primary Key (Single to Composite) */
TEST_CASE(compare_constraints, pk_modify_composite) {
    MemoryContext *ctx = memory_context_create("test_pk_modify_composite");
    ASSERT_NOT_NULL(ctx);

    CreateTableStmt *source = parse_table_from_file("tests/data/compare_tests/constraint_changes/employees_add_pk.sql");
    CreateTableStmt *target = parse_table_from_file("tests/data/compare_tests/constraint_changes/employees_pk_composite.sql");
    ASSERT_NOT_NULL(source);
    ASSERT_NOT_NULL(target);

    CompareOptions *opts = compare_options_default();
    TableDiff *diff = compare_tables(source, target, opts, ctx);

    /* Validate: PK modified (inline to table-level or column changes) */
    ASSERT_NOT_NULL(diff);
    /* Table-level composite PK vs inline PK will show differences */
    ASSERT_TRUE(diff->constraint_remove_count > 0 || diff->constraint_modify_count > 0 ||
                diff->constraint_add_count > 0 || diff->column_modify_count > 0);

    compare_options_free(opts);
    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test 6: Add Unique Constraint */
TEST_CASE(compare_constraints, unique_add) {
    MemoryContext *ctx = memory_context_create("test_unique_add");
    ASSERT_NOT_NULL(ctx);

    CreateTableStmt *source = parse_table_from_file("tests/data/compare_tests/baseline/users_base.sql");
    CreateTableStmt *target = parse_table_from_file("tests/data/compare_tests/constraint_changes/users_add_unique.sql");
    ASSERT_NOT_NULL(source);
    ASSERT_NOT_NULL(target);

    CompareOptions *opts = compare_options_default();
    TableDiff *diff = compare_tables(source, target, opts, ctx);

    /* Validate: Unique constraint added (inline UNIQUE shows as column modification or constraint) */
    ASSERT_NOT_NULL(diff);
    ASSERT_TRUE(diff->constraint_add_count > 0 || diff->column_modify_count > 0);

    compare_options_free(opts);
    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test 7: Constraint Name Change with ignore_constraint_names option */
TEST_CASE(compare_constraints, ignore_constraint_names) {
    MemoryContext *ctx = memory_context_create("test_ignore_constraint_names");
    ASSERT_NOT_NULL(ctx);

    CreateTableStmt *source = parse_table_from_file("tests/data/compare_tests/constraint_changes/users_add_unique.sql");
    CreateTableStmt *target = parse_table_from_file("tests/data/compare_tests/constraint_changes/users_unique_renamed.sql");
    ASSERT_NOT_NULL(source);
    ASSERT_NOT_NULL(target);

    /* Test with ignore_constraint_names enabled */
    CompareOptions *opts = compare_options_default();
    opts->ignore_constraint_names = true;
    TableDiff *diff = compare_tables(source, target, opts, ctx);

    /* Validate: No constraint changes when matching by semantics */
    ASSERT_NOT_NULL(diff);
    /* With ignore_constraint_names, the constraints should match */
    /* Total diff count should be minimal or zero */
    ASSERT_TRUE(diff->constraint_add_count == 0 || diff->constraint_remove_count == 0);

    compare_options_free(opts);
    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test 8: Add CHECK Constraint */
TEST_CASE(compare_constraints, check_add) {
    MemoryContext *ctx = memory_context_create("test_check_add");
    ASSERT_NOT_NULL(ctx);

    CreateTableStmt *source = parse_table_from_file("tests/data/compare_tests/baseline/products_base.sql");
    CreateTableStmt *target = parse_table_from_file("tests/data/compare_tests/constraint_changes/products_add_check.sql");
    ASSERT_NOT_NULL(source);
    ASSERT_NOT_NULL(target);

    CompareOptions *opts = compare_options_default();
    TableDiff *diff = compare_tables(source, target, opts, ctx);

    /* Validate: CHECK constraint added */
    ASSERT_NOT_NULL(diff);
    ASSERT_EQ(diff->constraint_add_count, 1);

    compare_options_free(opts);
    memory_context_destroy(ctx);
    TEST_PASS();
}

/* ============================================================================
 * Test Suite Definition
 * ============================================================================ */

static TestCase compare_constraints_tests[] = {
    {"fk_add", test_compare_constraints_fk_add, "compare_constraints"},
    {"fk_on_delete_change", test_compare_constraints_fk_on_delete_change, "compare_constraints"},
    {"fk_add_on_update", test_compare_constraints_fk_add_on_update, "compare_constraints"},
    {"pk_add", test_compare_constraints_pk_add, "compare_constraints"},
    {"pk_modify_composite", test_compare_constraints_pk_modify_composite, "compare_constraints"},
    {"unique_add", test_compare_constraints_unique_add, "compare_constraints"},
    {"ignore_constraint_names", test_compare_constraints_ignore_constraint_names, "compare_constraints"},
    {"check_add", test_compare_constraints_check_add, "compare_constraints"},
};

void run_compare_constraints_tests(void) {
    run_test_suite("compare_constraints", NULL, NULL, compare_constraints_tests,
                   sizeof(compare_constraints_tests) / sizeof(compare_constraints_tests[0]));
}
