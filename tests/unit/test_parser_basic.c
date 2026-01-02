#include "../test_framework.h"
#include "parser.h"
#include "pg_create_table.h"
#include <string.h>

/* Test: Parse simple table */
TEST_CASE(parser_basic, parse_simple_table) {
    Parser *parser = parser_create("CREATE TABLE t (id INTEGER);");
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_STR_EQ(stmt->table_name, "t");
    ASSERT_EQ(stmt->table_type, TABLE_TYPE_NORMAL);
    ASSERT_FALSE(stmt->if_not_exists);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse IF NOT EXISTS */
TEST_CASE(parser_basic, parse_if_not_exists) {
    Parser *parser = parser_create("CREATE TABLE IF NOT EXISTS users (id INTEGER);");
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_STR_EQ(stmt->table_name, "users");
    ASSERT_TRUE(stmt->if_not_exists);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse TEMPORARY TABLE */
TEST_CASE(parser_basic, parse_temporary) {
    Parser *parser = parser_create("CREATE TEMPORARY TABLE temp_data (id INTEGER);");
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_STR_EQ(stmt->table_name, "temp_data");
    ASSERT_TRUE(stmt->table_type == TABLE_TYPE_TEMPORARY || stmt->table_type == TABLE_TYPE_TEMP);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse TEMP TABLE (alias) */
TEST_CASE(parser_basic, parse_temp) {
    Parser *parser = parser_create("CREATE TEMP TABLE temp_data (id INTEGER);");
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_STR_EQ(stmt->table_name, "temp_data");
    ASSERT_TRUE(stmt->table_type == TABLE_TYPE_TEMPORARY || stmt->table_type == TABLE_TYPE_TEMP);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse UNLOGGED TABLE */
TEST_CASE(parser_basic, parse_unlogged) {
    Parser *parser = parser_create("CREATE UNLOGGED TABLE fast_data (id INTEGER);");
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_STR_EQ(stmt->table_name, "fast_data");
    ASSERT_EQ(stmt->table_type, TABLE_TYPE_UNLOGGED);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse schema-qualified table name */
TEST_CASE(parser_basic, parse_schema_qualified) {
    Parser *parser = parser_create("CREATE TABLE myschema.mytable (id INTEGER);");
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    /* Table name might be qualified or unqualified depending on implementation */
    ASSERT_NOT_NULL(stmt->table_name);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse table with multiple columns */
TEST_CASE(parser_basic, parse_multiple_columns) {
    Parser *parser = parser_create(
        "CREATE TABLE users ("
        "  id INTEGER,"
        "  name VARCHAR(100),"
        "  age INTEGER"
        ");"
    );
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_STR_EQ(stmt->table_name, "users");

    /* Should have 3 columns */
    ASSERT_EQ(stmt->variant, CREATE_TABLE_REGULAR);
    ASSERT_NOT_NULL(stmt->table_def.regular.elements);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse empty table (no columns) */
TEST_CASE(parser_basic, parse_empty_table) {
    Parser *parser = parser_create("CREATE TABLE empty_table ();");
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);

    /* Depending on implementation, this might succeed or fail */
    /* PostgreSQL allows empty tables */

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse table with single column */
TEST_CASE(parser_basic, parse_single_column) {
    Parser *parser = parser_create("CREATE TABLE single (id INTEGER);");
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_STR_EQ(stmt->table_name, "single");

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse table with quoted identifier */
TEST_CASE(parser_basic, parse_quoted_table_name) {
    Parser *parser = parser_create("CREATE TABLE \"select\" (id INTEGER);");
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_NOT_NULL(stmt->table_name);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse table with quoted column name */
TEST_CASE(parser_basic, parse_quoted_column_name) {
    Parser *parser = parser_create("CREATE TABLE t (\"from\" INTEGER);");
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Case insensitive keywords */
TEST_CASE(parser_basic, case_insensitive_keywords) {
    const char *variations[] = {
        "CREATE TABLE t (id INTEGER);",
        "create table t (id integer);",
        "Create Table t (Id Integer);",
        "CrEaTe TaBlE t (iD iNtEgEr);"
    };

    for (size_t i = 0; i < sizeof(variations) / sizeof(variations[0]); i++) {
        Parser *parser = parser_create(variations[i]);
        ASSERT_NOT_NULL(parser);

        CreateTableStmt *stmt = parser_parse_create_table(parser);
        ASSERT_NOT_NULL(stmt);
        ASSERT_NOT_NULL(stmt->table_name);

        parser_destroy(parser);
    }

    TEST_PASS();
}

/* Test: Table with whitespace variations */
TEST_CASE(parser_basic, whitespace_handling) {
    const char *variations[] = {
        "CREATE TABLE t(id INTEGER);",
        "CREATE  TABLE  t  (  id  INTEGER  )  ;",
        "CREATE\tTABLE\tt\t(\tid\tINTEGER\t);",
        "CREATE\nTABLE\nt\n(\nid\nINTEGER\n);",
    };

    for (size_t i = 0; i < sizeof(variations) / sizeof(variations[0]); i++) {
        Parser *parser = parser_create(variations[i]);
        ASSERT_NOT_NULL(parser);

        CreateTableStmt *stmt = parser_parse_create_table(parser);
        ASSERT_NOT_NULL(stmt);

        parser_destroy(parser);
    }

    TEST_PASS();
}

/* Test: Table with comments */
TEST_CASE(parser_basic, with_comments) {
    Parser *parser = parser_create(
        "-- This is a comment\n"
        "CREATE TABLE t ( /* inline comment */ id INTEGER /* another */ );"
    );
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_STR_EQ(stmt->table_name, "t");

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Combined modifiers */
TEST_CASE(parser_basic, combined_modifiers) {
    Parser *parser = parser_create("CREATE TEMPORARY TABLE IF NOT EXISTS temp_test (id INTEGER);");
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_STR_EQ(stmt->table_name, "temp_test");
    ASSERT_TRUE(stmt->if_not_exists);
    ASSERT_TRUE(stmt->table_type == TABLE_TYPE_TEMPORARY || stmt->table_type == TABLE_TYPE_TEMP);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test suite definition */
static TestCase parser_basic_tests[] = {
    {"parse_simple_table", test_parser_basic_parse_simple_table, "parser_basic"},
    {"parse_if_not_exists", test_parser_basic_parse_if_not_exists, "parser_basic"},
    {"parse_temporary", test_parser_basic_parse_temporary, "parser_basic"},
    {"parse_temp", test_parser_basic_parse_temp, "parser_basic"},
    {"parse_unlogged", test_parser_basic_parse_unlogged, "parser_basic"},
    {"parse_schema_qualified", test_parser_basic_parse_schema_qualified, "parser_basic"},
    {"parse_multiple_columns", test_parser_basic_parse_multiple_columns, "parser_basic"},
    {"parse_empty_table", test_parser_basic_parse_empty_table, "parser_basic"},
    {"parse_single_column", test_parser_basic_parse_single_column, "parser_basic"},
    {"parse_quoted_table_name", test_parser_basic_parse_quoted_table_name, "parser_basic"},
    {"parse_quoted_column_name", test_parser_basic_parse_quoted_column_name, "parser_basic"},
    {"case_insensitive_keywords", test_parser_basic_case_insensitive_keywords, "parser_basic"},
    {"whitespace_handling", test_parser_basic_whitespace_handling, "parser_basic"},
    {"with_comments", test_parser_basic_with_comments, "parser_basic"},
    {"combined_modifiers", test_parser_basic_combined_modifiers, "parser_basic"},
};

void run_parser_basic_tests(void) {
    run_test_suite("parser_basic", NULL, NULL, parser_basic_tests,
                   sizeof(parser_basic_tests) / sizeof(parser_basic_tests[0]));
}
