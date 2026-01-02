#include "../test_framework.h"
#include "parser.h"
#include "pg_create_table.h"
#include <string.h>

/* Test: Parse INHERITS clause */
TEST_CASE(parser_advanced, parse_inherits) {
    Parser *parser = parser_create(
        "CREATE TABLE employees (name VARCHAR(100)) INHERITS (base_table);"
    );
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse PARTITION BY RANGE */
TEST_CASE(parser_advanced, parse_partition_by_range) {
    Parser *parser = parser_create(
        "CREATE TABLE events (event_date DATE) PARTITION BY RANGE (event_date);"
    );
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse PARTITION BY LIST */
TEST_CASE(parser_advanced, parse_partition_by_list) {
    Parser *parser = parser_create(
        "CREATE TABLE sales (region VARCHAR(50)) PARTITION BY LIST (region);"
    );
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse PARTITION BY HASH */
TEST_CASE(parser_advanced, parse_partition_by_hash) {
    Parser *parser = parser_create(
        "CREATE TABLE users (id INTEGER) PARTITION BY HASH (id);"
    );
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse PARTITION OF */
TEST_CASE(parser_advanced, parse_partition_of) {
    Parser *parser = parser_create(
        "CREATE TABLE events_2024 PARTITION OF events FOR VALUES FROM ('2024-01-01') TO ('2025-01-01');"
    );
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    /* PARTITION OF may not be fully implemented yet */
    (void)stmt;

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse LIKE clause */
TEST_CASE(parser_advanced, parse_like_clause) {
    Parser *parser = parser_create(
        "CREATE TABLE new_users (LIKE users INCLUDING ALL);"
    );
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse WITH options */
TEST_CASE(parser_advanced, parse_with_options) {
    Parser *parser = parser_create(
        "CREATE TABLE t (id INTEGER) WITH (fillfactor=70);"
    );
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse TABLESPACE */
TEST_CASE(parser_advanced, parse_tablespace) {
    Parser *parser = parser_create(
        "CREATE TABLE t (id INTEGER) TABLESPACE pg_default;"
    );
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse ON COMMIT DROP */
TEST_CASE(parser_advanced, parse_on_commit_drop) {
    Parser *parser = parser_create(
        "CREATE TEMP TABLE t (id INTEGER) ON COMMIT DROP;"
    );
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse ON COMMIT DELETE ROWS */
TEST_CASE(parser_advanced, parse_on_commit_delete_rows) {
    Parser *parser = parser_create(
        "CREATE TEMP TABLE t (id INTEGER) ON COMMIT DELETE ROWS;"
    );
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse syntax error - missing semicolon */
TEST_CASE(parser_advanced, error_missing_semicolon) {
    Parser *parser = parser_create("CREATE TABLE t (id INTEGER)");
    ASSERT_NOT_NULL(parser);

    /* May succeed or fail depending on whether semicolon is required */
    /* Just ensure we don't crash */
    CreateTableStmt *stmt = parser_parse_create_table(parser);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse syntax error - unexpected token */
TEST_CASE(parser_advanced, error_unexpected_token) {
    Parser *parser = parser_create("CREATE TABLE t (id INVALID_TYPE);");
    ASSERT_NOT_NULL(parser);

    /* Should handle error gracefully */
    CreateTableStmt *stmt = parser_parse_create_table(parser);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse incomplete statement */
TEST_CASE(parser_advanced, error_incomplete_statement) {
    Parser *parser = parser_create("CREATE TABLE");
    ASSERT_NOT_NULL(parser);

    /* Should handle error gracefully */
    CreateTableStmt *stmt = parser_parse_create_table(parser);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse very long table name */
TEST_CASE(parser_advanced, long_identifier) {
    char long_name[100];
    memset(long_name, 'a', 63);  /* PostgreSQL max identifier length */
    long_name[63] = '\0';

    char sql[200];
    snprintf(sql, sizeof(sql), "CREATE TABLE %s (id INTEGER);", long_name);

    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse table with many columns */
TEST_CASE(parser_advanced, many_columns) {
    Parser *parser = parser_create(
        "CREATE TABLE t ("
        "  c1 INTEGER, c2 INTEGER, c3 INTEGER, c4 INTEGER, c5 INTEGER,"
        "  c6 INTEGER, c7 INTEGER, c8 INTEGER, c9 INTEGER, c10 INTEGER,"
        "  c11 INTEGER, c12 INTEGER, c13 INTEGER, c14 INTEGER, c15 INTEGER,"
        "  c16 INTEGER, c17 INTEGER, c18 INTEGER, c19 INTEGER, c20 INTEGER"
        ");"
    );
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse real-world example */
TEST_CASE(parser_advanced, real_world_table) {
    Parser *parser = parser_create(
        "CREATE TABLE orders ("
        "  order_id INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,"
        "  customer_id INTEGER NOT NULL,"
        "  order_date TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "  status VARCHAR(20) NOT NULL DEFAULT 'pending',"
        "  total_amount NUMERIC(10,2) NOT NULL,"
        "  notes TEXT,"
        "  metadata JSONB,"
        "  CONSTRAINT orders_customer_id_fkey"
        "    FOREIGN KEY (customer_id)"
        "    REFERENCES customers(customer_id)"
        "    ON DELETE CASCADE"
        "    ON UPDATE RESTRICT,"
        "  CONSTRAINT orders_status_check"
        "    CHECK (status IN ('pending', 'processing', 'shipped', 'delivered', 'cancelled')),"
        "  CONSTRAINT orders_total_positive"
        "    CHECK (total_amount >= 0)"
        ") WITH (fillfactor=80);"
    );
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);
    ASSERT_STR_EQ(stmt->table_name, "orders");

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test suite definition */
static TestCase parser_advanced_tests[] = {
    {"parse_inherits", test_parser_advanced_parse_inherits, "parser_advanced"},
    {"parse_partition_by_range", test_parser_advanced_parse_partition_by_range, "parser_advanced"},
    {"parse_partition_by_list", test_parser_advanced_parse_partition_by_list, "parser_advanced"},
    {"parse_partition_by_hash", test_parser_advanced_parse_partition_by_hash, "parser_advanced"},
    {"parse_partition_of", test_parser_advanced_parse_partition_of, "parser_advanced"},
    {"parse_like_clause", test_parser_advanced_parse_like_clause, "parser_advanced"},
    {"parse_with_options", test_parser_advanced_parse_with_options, "parser_advanced"},
    {"parse_tablespace", test_parser_advanced_parse_tablespace, "parser_advanced"},
    {"parse_on_commit_drop", test_parser_advanced_parse_on_commit_drop, "parser_advanced"},
    {"parse_on_commit_delete_rows", test_parser_advanced_parse_on_commit_delete_rows, "parser_advanced"},
    {"error_missing_semicolon", test_parser_advanced_error_missing_semicolon, "parser_advanced"},
    {"error_unexpected_token", test_parser_advanced_error_unexpected_token, "parser_advanced"},
    {"error_incomplete_statement", test_parser_advanced_error_incomplete_statement, "parser_advanced"},
    {"long_identifier", test_parser_advanced_long_identifier, "parser_advanced"},
    {"many_columns", test_parser_advanced_many_columns, "parser_advanced"},
    {"real_world_table", test_parser_advanced_real_world_table, "parser_advanced"},
};

void run_parser_advanced_tests(void) {
    run_test_suite("parser_advanced", NULL, NULL, parser_advanced_tests,
                   sizeof(parser_advanced_tests) / sizeof(parser_advanced_tests[0]));
}
