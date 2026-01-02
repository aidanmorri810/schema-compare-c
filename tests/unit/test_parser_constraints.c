#include "../test_framework.h"
#include "parser.h"
#include "pg_create_table.h"
#include <string.h>

/* Test: Parse PRIMARY KEY column constraint */
TEST_CASE(parser_constraints, parse_primary_key_column) {
    Parser *parser = parser_create("CREATE TABLE t (id INTEGER PRIMARY KEY);");
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse PRIMARY KEY table constraint */
TEST_CASE(parser_constraints, parse_primary_key_table) {
    Parser *parser = parser_create(
        "CREATE TABLE t (id INTEGER, name VARCHAR(50), PRIMARY KEY (id));"
    );
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse composite PRIMARY KEY */
TEST_CASE(parser_constraints, parse_composite_primary_key) {
    Parser *parser = parser_create(
        "CREATE TABLE t (user_id INTEGER, group_id INTEGER, PRIMARY KEY (user_id, group_id));"
    );
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse FOREIGN KEY */
TEST_CASE(parser_constraints, parse_foreign_key) {
    Parser *parser = parser_create(
        "CREATE TABLE t (user_id INTEGER REFERENCES users(id));"
    );
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse FOREIGN KEY table constraint */
TEST_CASE(parser_constraints, parse_foreign_key_table) {
    Parser *parser = parser_create(
        "CREATE TABLE t (user_id INTEGER, FOREIGN KEY (user_id) REFERENCES users(id));"
    );
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse ON DELETE CASCADE */
TEST_CASE(parser_constraints, parse_on_delete_cascade) {
    Parser *parser = parser_create(
        "CREATE TABLE t (user_id INTEGER REFERENCES users(id) ON DELETE CASCADE);"
    );
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse ON UPDATE RESTRICT */
TEST_CASE(parser_constraints, parse_on_update_restrict) {
    Parser *parser = parser_create(
        "CREATE TABLE t (user_id INTEGER REFERENCES users(id) ON UPDATE RESTRICT);"
    );
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse ON DELETE SET NULL */
TEST_CASE(parser_constraints, parse_on_delete_set_null) {
    Parser *parser = parser_create(
        "CREATE TABLE t (user_id INTEGER REFERENCES users(id) ON DELETE SET NULL);"
    );
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse UNIQUE constraint */
TEST_CASE(parser_constraints, parse_unique) {
    Parser *parser = parser_create("CREATE TABLE t (email VARCHAR(100) UNIQUE);");
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse UNIQUE table constraint */
TEST_CASE(parser_constraints, parse_unique_table) {
    Parser *parser = parser_create(
        "CREATE TABLE t (email VARCHAR(100), UNIQUE (email));"
    );
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse CHECK constraint */
TEST_CASE(parser_constraints, parse_check) {
    Parser *parser = parser_create(
        "CREATE TABLE t (age INTEGER CHECK (age >= 0));"
    );
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse CHECK table constraint */
TEST_CASE(parser_constraints, parse_check_table) {
    Parser *parser = parser_create(
        "CREATE TABLE t (age INTEGER, CHECK (age >= 0 AND age <= 150));"
    );
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse named constraint */
TEST_CASE(parser_constraints, parse_named_constraint) {
    Parser *parser = parser_create(
        "CREATE TABLE t (id INTEGER, CONSTRAINT pk_t PRIMARY KEY (id));"
    );
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse DEFERRABLE constraint */
TEST_CASE(parser_constraints, parse_deferrable) {
    Parser *parser = parser_create(
        "CREATE TABLE t (id INTEGER, CONSTRAINT pk_t PRIMARY KEY (id) DEFERRABLE);"
    );
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse INITIALLY DEFERRED */
TEST_CASE(parser_constraints, parse_initially_deferred) {
    Parser *parser = parser_create(
        "CREATE TABLE t (id INTEGER, CONSTRAINT pk_t PRIMARY KEY (id) INITIALLY DEFERRED);"
    );
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse INITIALLY IMMEDIATE */
TEST_CASE(parser_constraints, parse_initially_immediate) {
    Parser *parser = parser_create(
        "CREATE TABLE t (id INTEGER, CONSTRAINT pk_t PRIMARY KEY (id) INITIALLY IMMEDIATE);"
    );
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse NOT ENFORCED */
TEST_CASE(parser_constraints, parse_not_enforced) {
    Parser *parser = parser_create(
        "CREATE TABLE t (age INTEGER CHECK (age >= 0) NOT ENFORCED);"
    );
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse MATCH FULL */
TEST_CASE(parser_constraints, parse_match_full) {
    Parser *parser = parser_create(
        "CREATE TABLE t (user_id INTEGER REFERENCES users(id) MATCH FULL);"
    );
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse MATCH SIMPLE */
TEST_CASE(parser_constraints, parse_match_simple) {
    Parser *parser = parser_create(
        "CREATE TABLE t (user_id INTEGER REFERENCES users(id) MATCH SIMPLE);"
    );
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test: Parse complex constraint combination */
TEST_CASE(parser_constraints, parse_complex_constraints) {
    Parser *parser = parser_create(
        "CREATE TABLE orders ("
        "  order_id INTEGER PRIMARY KEY,"
        "  customer_id INTEGER NOT NULL REFERENCES customers(id) ON DELETE CASCADE,"
        "  total NUMERIC(10,2) CHECK (total >= 0),"
        "  status VARCHAR(20) DEFAULT 'pending' CHECK (status IN ('pending', 'shipped')),"
        "  UNIQUE (customer_id, order_id)"
        ");"
    );
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    TEST_PASS();
}

/* Test suite definition */
static TestCase parser_constraints_tests[] = {
    {"parse_primary_key_column", test_parser_constraints_parse_primary_key_column, "parser_constraints"},
    {"parse_primary_key_table", test_parser_constraints_parse_primary_key_table, "parser_constraints"},
    {"parse_composite_primary_key", test_parser_constraints_parse_composite_primary_key, "parser_constraints"},
    {"parse_foreign_key", test_parser_constraints_parse_foreign_key, "parser_constraints"},
    {"parse_foreign_key_table", test_parser_constraints_parse_foreign_key_table, "parser_constraints"},
    {"parse_on_delete_cascade", test_parser_constraints_parse_on_delete_cascade, "parser_constraints"},
    {"parse_on_update_restrict", test_parser_constraints_parse_on_update_restrict, "parser_constraints"},
    {"parse_on_delete_set_null", test_parser_constraints_parse_on_delete_set_null, "parser_constraints"},
    {"parse_unique", test_parser_constraints_parse_unique, "parser_constraints"},
    {"parse_unique_table", test_parser_constraints_parse_unique_table, "parser_constraints"},
    {"parse_check", test_parser_constraints_parse_check, "parser_constraints"},
    {"parse_check_table", test_parser_constraints_parse_check_table, "parser_constraints"},
    {"parse_named_constraint", test_parser_constraints_parse_named_constraint, "parser_constraints"},
    {"parse_deferrable", test_parser_constraints_parse_deferrable, "parser_constraints"},
    {"parse_initially_deferred", test_parser_constraints_parse_initially_deferred, "parser_constraints"},
    {"parse_initially_immediate", test_parser_constraints_parse_initially_immediate, "parser_constraints"},
    {"parse_not_enforced", test_parser_constraints_parse_not_enforced, "parser_constraints"},
    {"parse_match_full", test_parser_constraints_parse_match_full, "parser_constraints"},
    {"parse_match_simple", test_parser_constraints_parse_match_simple, "parser_constraints"},
    {"parse_complex_constraints", test_parser_constraints_parse_complex_constraints, "parser_constraints"},
};

void run_parser_constraints_tests(void) {
    run_test_suite("parser_constraints", NULL, NULL, parser_constraints_tests,
                   sizeof(parser_constraints_tests) / sizeof(parser_constraints_tests[0]));
}
