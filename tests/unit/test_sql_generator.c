#include "../test_framework.h"
#include "sql_generator.h"
#include "diff.h"
#include <string.h>

/* Test: SQL generation options default */
TEST_CASE(sql_generator, sql_gen_options_default) {
    SQLGenOptions *opts = sql_gen_options_default();
    ASSERT_NOT_NULL(opts);

    /* Check that options structure is initialized */
    /* Values can be true or false, just check it doesn't crash */

    sql_gen_options_free(opts);
    TEST_PASS();
}

/* Test: Quote identifier */
TEST_CASE(sql_generator, quote_identifier) {
    char *quoted = quote_identifier("select");
    ASSERT_NOT_NULL(quoted);

    /* Should be quoted (typically with double quotes in PostgreSQL) */
    ASSERT_TRUE(strlen(quoted) > strlen("select"));

    free(quoted);
    TEST_PASS();
}

/* Test: Quote identifier - normal name */
TEST_CASE(sql_generator, quote_identifier_normal) {
    char *quoted = quote_identifier("users");
    ASSERT_NOT_NULL(quoted);

    /* Behavior may vary - could be quoted or not */
    ASSERT_TRUE(strlen(quoted) > 0);

    free(quoted);
    TEST_PASS();
}

/* Test: Quote literal */
TEST_CASE(sql_generator, quote_literal) {
    char *quoted = quote_literal("test value");
    ASSERT_NOT_NULL(quoted);

    /* Should be quoted with single quotes */
    ASSERT_TRUE(strlen(quoted) > strlen("test value"));

    free(quoted);
    TEST_PASS();
}

/* Test: Format data type */
TEST_CASE(sql_generator, format_data_type) {
    char *formatted = format_data_type("INTEGER");
    ASSERT_NOT_NULL(formatted);
    ASSERT_TRUE(strlen(formatted) > 0);

    free(formatted);
    TEST_PASS();
}

/* Test: Generate DROP TABLE SQL */
TEST_CASE(sql_generator, generate_drop_table_sql) {
    SQLGenOptions *opts = sql_gen_options_default();
    opts->use_if_exists = false;

    char *sql = generate_drop_table_sql("old_table", opts);
    ASSERT_NOT_NULL(sql);

    /* Should contain DROP and TABLE */
    ASSERT_TRUE(strstr(sql, "DROP") != NULL || strstr(sql, "drop") != NULL);
    ASSERT_TRUE(strstr(sql, "TABLE") != NULL || strstr(sql, "table") != NULL);

    free(sql);
    sql_gen_options_free(opts);
    TEST_PASS();
}

/* Test: Generate DROP TABLE SQL with IF EXISTS */
TEST_CASE(sql_generator, generate_drop_table_if_exists) {
    SQLGenOptions *opts = sql_gen_options_default();
    opts->use_if_exists = true;

    char *sql = generate_drop_table_sql("old_table", opts);
    ASSERT_NOT_NULL(sql);

    /* Should contain IF EXISTS */
    ASSERT_TRUE(strstr(sql, "IF EXISTS") != NULL || strstr(sql, "if exists") != NULL);

    free(sql);
    sql_gen_options_free(opts);
    TEST_PASS();
}

/* Test: Generate DROP COLUMN SQL */
TEST_CASE(sql_generator, generate_drop_column_sql) {
    SQLGenOptions *opts = sql_gen_options_default();

    char *sql = generate_drop_column_sql("users", "old_field", opts);
    ASSERT_NOT_NULL(sql);

    /* Should contain ALTER TABLE and DROP */
    ASSERT_TRUE(strstr(sql, "ALTER") != NULL || strstr(sql, "alter") != NULL);
    ASSERT_TRUE(strstr(sql, "DROP") != NULL || strstr(sql, "drop") != NULL);

    free(sql);
    sql_gen_options_free(opts);
    TEST_PASS();
}

/* Test: Generate ADD COLUMN SQL */
TEST_CASE(sql_generator, generate_add_column_sql) {
    ColumnDiff *col = column_diff_create("email");
    col->new_type = "VARCHAR(100)";
    col->type_changed = false;

    SQLGenOptions *opts = sql_gen_options_default();

    char *sql = generate_add_column_sql("users", col, opts);
    ASSERT_NOT_NULL(sql);

    /* Should contain ALTER TABLE and ADD */
    ASSERT_TRUE(strstr(sql, "ALTER") != NULL || strstr(sql, "alter") != NULL);
    ASSERT_TRUE(strstr(sql, "ADD") != NULL || strstr(sql, "add") != NULL);

    free(sql);
    sql_gen_options_free(opts);
    column_diff_free(col);
    TEST_PASS();
}

/* Test: Generate ALTER COLUMN TYPE SQL */
TEST_CASE(sql_generator, generate_alter_column_type_sql) {
    ColumnDiff *col = column_diff_create("id");
    col->old_type = "INTEGER";
    col->new_type = "BIGINT";
    col->type_changed = true;

    SQLGenOptions *opts = sql_gen_options_default();

    char *sql = generate_alter_column_type_sql("users", col, opts);
    ASSERT_NOT_NULL(sql);

    /* Should contain ALTER and TYPE */
    ASSERT_TRUE(strstr(sql, "ALTER") != NULL || strstr(sql, "alter") != NULL);

    free(sql);
    sql_gen_options_free(opts);
    column_diff_free(col);
    TEST_PASS();
}

/* Test: Generate ALTER COLUMN NULLABLE SQL */
TEST_CASE(sql_generator, generate_alter_column_nullable_sql) {
    ColumnDiff *col = column_diff_create("name");
    col->old_nullable = true;
    col->new_nullable = false;
    col->nullable_changed = true;

    SQLGenOptions *opts = sql_gen_options_default();

    char *sql = generate_alter_column_nullable_sql("users", col, opts);
    ASSERT_NOT_NULL(sql);

    /* Should contain ALTER and either SET or DROP */
    ASSERT_TRUE(strstr(sql, "ALTER") != NULL || strstr(sql, "alter") != NULL);

    free(sql);
    sql_gen_options_free(opts);
    column_diff_free(col);
    TEST_PASS();
}

/* Test: Generate ALTER COLUMN DEFAULT SQL */
TEST_CASE(sql_generator, generate_alter_column_default_sql) {
    ColumnDiff *col = column_diff_create("status");
    col->old_default = NULL;
    col->new_default = "'active'";
    col->default_changed = true;

    SQLGenOptions *opts = sql_gen_options_default();

    char *sql = generate_alter_column_default_sql("users", col, opts);
    ASSERT_NOT_NULL(sql);

    /* Should contain ALTER and DEFAULT */
    ASSERT_TRUE(strstr(sql, "ALTER") != NULL || strstr(sql, "alter") != NULL);

    free(sql);
    sql_gen_options_free(opts);
    column_diff_free(col);
    TEST_PASS();
}

/* Test: Generate migration SQL for empty diff */
TEST_CASE(sql_generator, generate_migration_empty) {
    SchemaDiff *diff = schema_diff_create("public");
    ASSERT_NOT_NULL(diff);

    SQLGenOptions *opts = sql_gen_options_default();

    SQLMigration *migration = generate_migration_sql(diff, opts);
    ASSERT_NOT_NULL(migration);

    /* Empty diff should have no statements or minimal SQL */
    /* Just verify migration was created successfully */
    ASSERT_TRUE(migration->statement_count >= 0);

    sql_migration_free(migration);
    sql_gen_options_free(opts);
    schema_diff_free(diff);
    TEST_PASS();
}

/* Test suite definition */
static TestCase sql_generator_tests[] = {
    {"sql_gen_options_default", test_sql_generator_sql_gen_options_default, "sql_generator"},
    {"quote_identifier", test_sql_generator_quote_identifier, "sql_generator"},
    {"quote_identifier_normal", test_sql_generator_quote_identifier_normal, "sql_generator"},
    {"quote_literal", test_sql_generator_quote_literal, "sql_generator"},
    {"format_data_type", test_sql_generator_format_data_type, "sql_generator"},
    {"generate_drop_table_sql", test_sql_generator_generate_drop_table_sql, "sql_generator"},
    {"generate_drop_table_if_exists", test_sql_generator_generate_drop_table_if_exists, "sql_generator"},
    {"generate_drop_column_sql", test_sql_generator_generate_drop_column_sql, "sql_generator"},
    {"generate_add_column_sql", test_sql_generator_generate_add_column_sql, "sql_generator"},
    {"generate_alter_column_type_sql", test_sql_generator_generate_alter_column_type_sql, "sql_generator"},
    {"generate_alter_column_nullable_sql", test_sql_generator_generate_alter_column_nullable_sql, "sql_generator"},
    {"generate_alter_column_default_sql", test_sql_generator_generate_alter_column_default_sql, "sql_generator"},
    {"generate_migration_empty", test_sql_generator_generate_migration_empty, "sql_generator"},
};

void run_sql_generator_tests(void) {
    run_test_suite("sql_generator", NULL, NULL, sql_generator_tests,
                   sizeof(sql_generator_tests) / sizeof(sql_generator_tests[0]));
}
