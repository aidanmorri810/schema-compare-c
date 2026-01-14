#include "../test_framework.h"
#include "compare.h"
#include "parser.h"
#include "pg_create_table.h"
#include "sc_memory.h"
#include <string.h>

/* Helper to create a test table */
static CreateTableStmt *create_test_table(const char *sql) {
    Parser *parser = parser_create(sql);
    if (!parser) {
        return NULL;
    }

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    parser_destroy(parser);

    return stmt;
}

/* Test: Compare options default */
TEST_CASE(compare, compare_options_default) {
    CompareOptions *opts = compare_options_default();
    ASSERT_NOT_NULL(opts);

    /* Check default values - these should be reasonable defaults */
    ASSERT_FALSE(opts->compare_tablespaces);
    ASSERT_TRUE(opts->compare_storage_params);
    ASSERT_TRUE(opts->compare_constraints);

    compare_options_free(opts);
    TEST_PASS();
}

/* Test: Normalize type names */
TEST_CASE(compare, normalize_type_name) {
    MemoryContext *ctx = memory_context_create("test_compare");
    ASSERT_NOT_NULL(ctx);

    /* Test that int4 and integer normalize to the same thing */
    char *norm1 = normalize_type_name("int4", ctx);
    char *norm2 = normalize_type_name("integer", ctx);

    ASSERT_NOT_NULL(norm1);
    ASSERT_NOT_NULL(norm2);
    /* Both should normalize to the same canonical form */
    ASSERT_STR_EQ(norm1, norm2);

    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test: Data types equal with normalization */
TEST_CASE(compare, data_types_equal_normalized) {
    CompareOptions *opts = compare_options_default();
    ASSERT_NOT_NULL(opts);

    opts->normalize_types = true;

    /* int4 and integer should be equal when normalized */
    bool equal = data_types_equal("int4", "integer", opts);
    ASSERT_TRUE(equal);

    compare_options_free(opts);
    TEST_PASS();
}

/* Test: Data types not equal */
TEST_CASE(compare, data_types_not_equal) {
    CompareOptions *opts = compare_options_default();
    ASSERT_NOT_NULL(opts);

    /* INTEGER and VARCHAR should not be equal */
    bool equal = data_types_equal("INTEGER", "VARCHAR", opts);
    ASSERT_FALSE(equal);

    compare_options_free(opts);
    TEST_PASS();
}

/* Test: Names equal case insensitive */
TEST_CASE(compare, names_equal_case_insensitive) {
    CompareOptions *opts = compare_options_default();
    ASSERT_NOT_NULL(opts);

    opts->case_sensitive = false;

    bool equal = names_equal("users", "USERS", opts);
    ASSERT_TRUE(equal);

    compare_options_free(opts);
    TEST_PASS();
}

/* Test: Names not equal case sensitive */
TEST_CASE(compare, names_not_equal_case_sensitive) {
    CompareOptions *opts = compare_options_default();
    ASSERT_NOT_NULL(opts);

    opts->case_sensitive = true;

    bool equal = names_equal("users", "USERS", opts);
    ASSERT_FALSE(equal);

    compare_options_free(opts);
    TEST_PASS();
}

/* Test: Expressions equal ignoring whitespace */
TEST_CASE(compare, expressions_equal_whitespace) {
    CompareOptions *opts = compare_options_default();
    ASSERT_NOT_NULL(opts);

    opts->ignore_whitespace = true;

    bool equal = expressions_equal("age >= 0", "age>=0", opts);
    ASSERT_TRUE(equal);

    compare_options_free(opts);
    TEST_PASS();
}

/* Test: Should compare table - no filters */
TEST_CASE(compare, should_compare_table_no_filters) {
    CompareOptions *opts = compare_options_default();
    ASSERT_NOT_NULL(opts);

    /* With no filters, all tables should be compared */
    bool should_compare = should_compare_table("users", opts);
    ASSERT_TRUE(should_compare);

    compare_options_free(opts);
    TEST_PASS();
}

/* Test: Compare identical tables */
TEST_CASE(compare, compare_identical_tables) {
    MemoryContext *ctx = memory_context_create("test_compare");
    ASSERT_NOT_NULL(ctx);

    CreateTableStmt *t1 = create_test_table("CREATE TABLE users (id INTEGER);");
    CreateTableStmt *t2 = create_test_table("CREATE TABLE users (id INTEGER);");

    ASSERT_NOT_NULL(t1);
    ASSERT_NOT_NULL(t2);

    CompareOptions *opts = compare_options_default();
    TableDiff *diff = compare_tables(t1, t2, opts, ctx);

    /* Identical tables should have no modifications */
    if (diff) {
        ASSERT_FALSE(diff->table_modified);
    }

    compare_options_free(opts);
    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test: Compare schemas - empty arrays */
TEST_CASE(compare, compare_schemas_empty) {
    MemoryContext *ctx = memory_context_create("test_compare");
    ASSERT_NOT_NULL(ctx);

    CompareOptions *opts = compare_options_default();

    /* Create empty schemas */
    Schema source = {0};
    Schema target = {0};

    SchemaDiff *diff = compare_schemas(&source, &target, opts, ctx);

    /* Comparing NULL/empty arrays may return NULL or an empty diff */
    if (diff) {
        /* If it returns a diff, it should have no changes */
        ASSERT_EQ(diff->total_diffs, 0);
        ASSERT_EQ(diff->tables_added, 0);
        ASSERT_EQ(diff->tables_removed, 0);
    }

    compare_options_free(opts);
    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test suite definition */
static TestCase compare_tests[] = {
    {"compare_options_default", test_compare_compare_options_default, "compare"},
    {"normalize_type_name", test_compare_normalize_type_name, "compare"},
    {"data_types_equal_normalized", test_compare_data_types_equal_normalized, "compare"},
    {"data_types_not_equal", test_compare_data_types_not_equal, "compare"},
    {"names_equal_case_insensitive", test_compare_names_equal_case_insensitive, "compare"},
    {"names_not_equal_case_sensitive", test_compare_names_not_equal_case_sensitive, "compare"},
    {"expressions_equal_whitespace", test_compare_expressions_equal_whitespace, "compare"},
    {"should_compare_table_no_filters", test_compare_should_compare_table_no_filters, "compare"},
    {"compare_identical_tables", test_compare_compare_identical_tables, "compare"},
    {"compare_schemas_empty", test_compare_compare_schemas_empty, "compare"},
};

void run_compare_tests(void) {
    run_test_suite("compare", NULL, NULL, compare_tests,
                   sizeof(compare_tests) / sizeof(compare_tests[0]));
}
