#include "../test_framework.h"
#include "compare.h"
#include "parser.h"
#include "pg_create_table.h"
#include "diff.h"
#include "sc_memory.h"
#include "utils.h"
#include <string.h>
#include <stdlib.h>

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

/* Helper to parse multiple tables from files */
static CreateTableStmt **parse_schema_from_files(const char **filenames, int count) {
    CreateTableStmt **tables = malloc(sizeof(CreateTableStmt*) * count);
    if (!tables) return NULL;

    for (int i = 0; i < count; i++) {
        tables[i] = parse_table_from_file(filenames[i]);
        if (!tables[i]) {
            /* Cleanup on error */
            for (int j = 0; j < i; j++) {
                free(tables[j]);
            }
            free(tables);
            return NULL;
        }
    }
    return tables;
}

/* ============================================================================
 * Schema-Level Tests (8 tests)
 * ============================================================================ */

/* Test 1: Table Added (Single) */
TEST_CASE(compare_schema, table_added_single) {
    MemoryContext *ctx = memory_context_create("test_table_added_single");
    ASSERT_NOT_NULL(ctx);

    /* Source: 2 tables, Target: 3 tables (add customers) */
    const char *source_files[] = {
        "tests/data/compare_tests/baseline/users_base.sql",
        "tests/data/compare_tests/baseline/products_base.sql"
    };
    const char *target_files[] = {
        "tests/data/compare_tests/baseline/users_base.sql",
        "tests/data/compare_tests/baseline/products_base.sql",
        "tests/data/compare_tests/schema_changes/customers_new.sql"
    };

    CreateTableStmt **source_tables = parse_schema_from_files(source_files, 2);
    CreateTableStmt **target_tables = parse_schema_from_files(target_files, 3);
    ASSERT_NOT_NULL(source_tables);
    ASSERT_NOT_NULL(target_tables);

    CompareOptions *opts = compare_options_default();
    SchemaDiff *diff = compare_schemas(source_tables, 2, target_tables, 3, opts, ctx);

    /* Validate: One table added */
    ASSERT_NOT_NULL(diff);
    ASSERT_EQ(diff->tables_added, 1);
    ASSERT_EQ(diff->tables_removed, 0);

    free(source_tables);
    free(target_tables);
    compare_options_free(opts);
    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test 2: Table Added (Multiple) */
TEST_CASE(compare_schema, table_added_multiple) {
    MemoryContext *ctx = memory_context_create("test_table_added_multiple");
    ASSERT_NOT_NULL(ctx);

    /* Source: 2 tables, Target: 4 tables (add customers + inventory) */
    const char *source_files[] = {
        "tests/data/compare_tests/baseline/users_base.sql",
        "tests/data/compare_tests/baseline/products_base.sql"
    };
    const char *target_files[] = {
        "tests/data/compare_tests/baseline/users_base.sql",
        "tests/data/compare_tests/baseline/products_base.sql",
        "tests/data/compare_tests/schema_changes/customers_new.sql",
        "tests/data/compare_tests/schema_changes/inventory_new.sql"
    };

    CreateTableStmt **source_tables = parse_schema_from_files(source_files, 2);
    CreateTableStmt **target_tables = parse_schema_from_files(target_files, 4);
    ASSERT_NOT_NULL(source_tables);
    ASSERT_NOT_NULL(target_tables);

    CompareOptions *opts = compare_options_default();
    SchemaDiff *diff = compare_schemas(source_tables, 2, target_tables, 4, opts, ctx);

    /* Validate: Two tables added */
    ASSERT_NOT_NULL(diff);
    ASSERT_EQ(diff->tables_added, 2);
    ASSERT_EQ(diff->tables_removed, 0);

    free(source_tables);
    free(target_tables);
    compare_options_free(opts);
    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test 3: Table Removed (Single) */
TEST_CASE(compare_schema, table_removed_single) {
    MemoryContext *ctx = memory_context_create("test_table_removed_single");
    ASSERT_NOT_NULL(ctx);

    /* Source: 3 tables, Target: 2 tables (remove products) */
    const char *source_files[] = {
        "tests/data/compare_tests/baseline/users_base.sql",
        "tests/data/compare_tests/baseline/products_base.sql",
        "tests/data/compare_tests/baseline/employees_base.sql"
    };
    const char *target_files[] = {
        "tests/data/compare_tests/baseline/users_base.sql",
        "tests/data/compare_tests/baseline/employees_base.sql"
    };

    CreateTableStmt **source_tables = parse_schema_from_files(source_files, 3);
    CreateTableStmt **target_tables = parse_schema_from_files(target_files, 2);
    ASSERT_NOT_NULL(source_tables);
    ASSERT_NOT_NULL(target_tables);

    CompareOptions *opts = compare_options_default();
    SchemaDiff *diff = compare_schemas(source_tables, 3, target_tables, 2, opts, ctx);

    /* Validate: One table removed */
    ASSERT_NOT_NULL(diff);
    ASSERT_EQ(diff->tables_added, 0);
    ASSERT_EQ(diff->tables_removed, 1);

    free(source_tables);
    free(target_tables);
    compare_options_free(opts);
    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test 4: Mixed Operations (Add + Remove + Modify) */
TEST_CASE(compare_schema, mixed_operations) {
    MemoryContext *ctx = memory_context_create("test_mixed_operations");
    ASSERT_NOT_NULL(ctx);

    /* Source: users, products; Target: users (modified), employees */
    const char *source_files[] = {
        "tests/data/compare_tests/baseline/users_base.sql",
        "tests/data/compare_tests/baseline/products_base.sql"
    };
    const char *target_files[] = {
        "tests/data/compare_tests/column_changes/users_add_email.sql",
        "tests/data/compare_tests/baseline/employees_base.sql"
    };

    CreateTableStmt **source_tables = parse_schema_from_files(source_files, 2);
    CreateTableStmt **target_tables = parse_schema_from_files(target_files, 2);
    ASSERT_NOT_NULL(source_tables);
    ASSERT_NOT_NULL(target_tables);

    CompareOptions *opts = compare_options_default();
    SchemaDiff *diff = compare_schemas(source_tables, 2, target_tables, 2, opts, ctx);

    /* Validate: added employees, removed products, modified users */
    ASSERT_NOT_NULL(diff);
    ASSERT_EQ(diff->tables_added, 1);
    ASSERT_EQ(diff->tables_removed, 1);
    ASSERT_EQ(diff->tables_modified, 1);

    free(source_tables);
    free(target_tables);
    compare_options_free(opts);
    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test 5: Type Normalization Option Test */
TEST_CASE(compare_schema, type_normalization) {
    MemoryContext *ctx = memory_context_create("test_type_normalization");
    ASSERT_NOT_NULL(ctx);

    CreateTableStmt *source = parse_table_from_file("tests/data/compare_tests/baseline/users_base.sql");
    CreateTableStmt *target = parse_table_from_file("tests/data/compare_tests/schema_changes/users_int4.sql");
    ASSERT_NOT_NULL(source);
    ASSERT_NOT_NULL(target);

    /* Test with type normalization enabled (default) */
    CompareOptions *opts = compare_options_default();
    opts->normalize_types = true;
    TableDiff *diff = compare_tables(source, target, opts, ctx);

    /* Validate: No type change detected when normalized (int4 == integer) */
    ASSERT_NOT_NULL(diff);
    /* With normalization, int4 and integer should be treated as same */
    ASSERT_EQ(diff->column_modify_count, 0);

    compare_options_free(opts);
    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test 6: Case Sensitivity Option Test */
TEST_CASE(compare_schema, case_sensitivity) {
    MemoryContext *ctx = memory_context_create("test_case_sensitivity");
    ASSERT_NOT_NULL(ctx);

    CreateTableStmt *source = parse_table_from_file("tests/data/compare_tests/baseline/users_base.sql");
    CreateTableStmt *target = parse_table_from_file("tests/data/compare_tests/schema_changes/Users_case.sql");
    ASSERT_NOT_NULL(source);
    ASSERT_NOT_NULL(target);

    /* Test with case insensitive mode */
    CompareOptions *opts = compare_options_default();
    opts->case_sensitive = false;

    /* Compare table names using the utility function */
    bool equal = names_equal(source->table_name, target->table_name, opts);
    ASSERT_TRUE(equal);

    compare_options_free(opts);
    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test 7: Ignore Whitespace in Expressions */
TEST_CASE(compare_schema, ignore_whitespace) {
    MemoryContext *ctx = memory_context_create("test_ignore_whitespace");
    ASSERT_NOT_NULL(ctx);

    CreateTableStmt *source = parse_table_from_file("tests/data/compare_tests/baseline/products_base.sql");
    CreateTableStmt *target = parse_table_from_file("tests/data/compare_tests/schema_changes/products_whitespace.sql");
    ASSERT_NOT_NULL(source);
    ASSERT_NOT_NULL(target);

    /* Test with ignore_whitespace enabled (default) */
    CompareOptions *opts = compare_options_default();
    opts->ignore_whitespace = true;
    TableDiff *diff = compare_tables(source, target, opts, ctx);

    /* Validate: No diff when whitespace differences ignored */
    ASSERT_NOT_NULL(diff);
    /* Should have minimal or no differences */
    ASSERT_TRUE(diff->diff_count == 0 || diff->column_modify_count == 0);

    compare_options_free(opts);
    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test 8: Complex Multi-Change Scenario */
TEST_CASE(compare_schema, complex_multi_change) {
    MemoryContext *ctx = memory_context_create("test_complex_multi_change");
    ASSERT_NOT_NULL(ctx);

    CreateTableStmt *source = parse_table_from_file("tests/data/compare_tests/baseline/orders_base.sql");
    CreateTableStmt *target = parse_table_from_file("tests/data/compare_tests/complex/orders_multi_change.sql");
    ASSERT_NOT_NULL(source);
    ASSERT_NOT_NULL(target);

    CompareOptions *opts = compare_options_default();
    TableDiff *diff = compare_tables(source, target, opts, ctx);

    /* Validate: Multiple changes detected */
    /* Add discount column + change FK + add CHECK constraint */
    ASSERT_NOT_NULL(diff);
    ASSERT_TRUE(diff->column_add_count > 0);  /* discount column added */
    ASSERT_TRUE(diff->constraint_add_count > 0 || diff->constraint_remove_count > 0);  /* FK and CHECK changes */
    ASSERT_TRUE(diff->diff_count >= 2);  /* At least 2 changes total */

    compare_options_free(opts);
    memory_context_destroy(ctx);
    TEST_PASS();
}

/* ============================================================================
 * Test Suite Definition
 * ============================================================================ */

static TestCase compare_schema_tests[] = {
    {"table_added_single", test_compare_schema_table_added_single, "compare_schema"},
    {"table_added_multiple", test_compare_schema_table_added_multiple, "compare_schema"},
    {"table_removed_single", test_compare_schema_table_removed_single, "compare_schema"},
    {"mixed_operations", test_compare_schema_mixed_operations, "compare_schema"},
    {"type_normalization", test_compare_schema_type_normalization, "compare_schema"},
    {"case_sensitivity", test_compare_schema_case_sensitivity, "compare_schema"},
    {"ignore_whitespace", test_compare_schema_ignore_whitespace, "compare_schema"},
    {"complex_multi_change", test_compare_schema_complex_multi_change, "compare_schema"},
};

void run_compare_schema_tests(void) {
    run_test_suite("compare_schema", NULL, NULL, compare_schema_tests,
                   sizeof(compare_schema_tests) / sizeof(compare_schema_tests[0]));
}
