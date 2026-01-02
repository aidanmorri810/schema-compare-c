#include "../test_framework.h"
#include "parser.h"
#include "pg_create_table.h"
#include "utils.h"
#include <string.h>

/* Test: Parse real Sakila actor table */
TEST_CASE(parser_integration, parse_sakila_actor) {
    char *sql = read_file_to_string("tests/data/silka/tables/actor.sql");
    ASSERT_NOT_NULL(sql);

    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    /* Verify table name */
    ASSERT_STR_EQ(stmt->table_name, "actor");

    /* Verify it's a regular table */
    ASSERT_EQ(stmt->table_type, TABLE_TYPE_NORMAL);

    /* Should have columns */
    ASSERT_EQ(stmt->variant, CREATE_TABLE_REGULAR);
    ASSERT_NOT_NULL(stmt->table_def.regular.elements);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    free(sql);
    TEST_PASS();
}

/* Test: Parse real Sakila film table */
TEST_CASE(parser_integration, parse_sakila_film) {
    char *sql = read_file_to_string("tests/data/silka/tables/film.sql");
    ASSERT_NOT_NULL(sql);

    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    /* Verify table name */
    ASSERT_STR_EQ(stmt->table_name, "film");

    /* Should have columns and constraints */
    ASSERT_EQ(stmt->variant, CREATE_TABLE_REGULAR);
    ASSERT_NOT_NULL(stmt->table_def.regular.elements);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    free(sql);
    TEST_PASS();
}

/* Test: Parse real Sakila customer table */
TEST_CASE(parser_integration, parse_sakila_customer) {
    char *sql = read_file_to_string("tests/data/silka/tables/customer.sql");
    ASSERT_NOT_NULL(sql);

    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    ASSERT_STR_EQ(stmt->table_name, "customer");
    ASSERT_EQ(stmt->variant, CREATE_TABLE_REGULAR);

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    free(sql);
    TEST_PASS();
}

/* Test: Parse real Sakila payment table */
TEST_CASE(parser_integration, parse_sakila_payment) {
    char *sql = read_file_to_string("tests/data/silka/tables/payment.sql");
    ASSERT_NOT_NULL(sql);

    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    ASSERT_STR_EQ(stmt->table_name, "payment");

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    free(sql);
    TEST_PASS();
}

/* Test: Parse real Sakila rental table */
TEST_CASE(parser_integration, parse_sakila_rental) {
    char *sql = read_file_to_string("tests/data/silka/tables/rental.sql");
    ASSERT_NOT_NULL(sql);

    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    ASSERT_STR_EQ(stmt->table_name, "rental");

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    free(sql);
    TEST_PASS();
}

/* Test: Parse real Sakila inventory table */
TEST_CASE(parser_integration, parse_sakila_inventory) {
    char *sql = read_file_to_string("tests/data/silka/tables/inventory.sql");
    ASSERT_NOT_NULL(sql);

    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    ASSERT_STR_EQ(stmt->table_name, "inventory");

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    free(sql);
    TEST_PASS();
}

/* Test: Parse real Sakila address table */
TEST_CASE(parser_integration, parse_sakila_address) {
    char *sql = read_file_to_string("tests/data/silka/tables/address.sql");
    ASSERT_NOT_NULL(sql);

    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    ASSERT_STR_EQ(stmt->table_name, "address");

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    free(sql);
    TEST_PASS();
}

/* Test: Parse real Sakila city table */
TEST_CASE(parser_integration, parse_sakila_city) {
    char *sql = read_file_to_string("tests/data/silka/tables/city.sql");
    ASSERT_NOT_NULL(sql);

    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    ASSERT_STR_EQ(stmt->table_name, "city");

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    free(sql);
    TEST_PASS();
}

/* Test: Parse real Sakila country table */
TEST_CASE(parser_integration, parse_sakila_country) {
    char *sql = read_file_to_string("tests/data/silka/tables/country.sql");
    ASSERT_NOT_NULL(sql);

    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    ASSERT_STR_EQ(stmt->table_name, "country");

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    free(sql);
    TEST_PASS();
}

/* Test: Parse real Sakila language table */
TEST_CASE(parser_integration, parse_sakila_language) {
    char *sql = read_file_to_string("tests/data/silka/tables/language.sql");
    ASSERT_NOT_NULL(sql);

    Parser *parser = parser_create(sql);
    ASSERT_NOT_NULL(parser);

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    ASSERT_NOT_NULL(stmt);

    ASSERT_STR_EQ(stmt->table_name, "language");

    parser_destroy(parser);
    free_create_table_stmt(stmt);
    free(sql);
    TEST_PASS();
}

/* Test: Parse all Sakila tables */
TEST_CASE(parser_integration, parse_all_sakila_tables) {
    const char *tables[] = {
        "tests/data/silka/tables/actor.sql",
        "tests/data/silka/tables/address.sql",
        "tests/data/silka/tables/city.sql",
        "tests/data/silka/tables/country.sql",
        "tests/data/silka/tables/customer.sql",
        "tests/data/silka/tables/film.sql",
        "tests/data/silka/tables/film_actor.sql",
        "tests/data/silka/tables/film_category.sql",
        "tests/data/silka/tables/inventory.sql",
        "tests/data/silka/tables/language.sql",
        "tests/data/silka/tables/payment.sql",
        "tests/data/silka/tables/rental.sql",
        "tests/data/silka/tables/staff.sql",
        "tests/data/silka/tables/store.sql",
    };

    int parsed_count = 0;
    size_t table_count = sizeof(tables) / sizeof(tables[0]);

    for (size_t i = 0; i < table_count; i++) {
        char *sql = read_file_to_string(tables[i]);
        if (!sql) {
            continue; /* Skip if file doesn't exist */
        }

        Parser *parser = parser_create(sql);
        if (parser) {
            CreateTableStmt *stmt = parser_parse_create_table(parser);
            if (stmt) {
                parsed_count++;
                free_create_table_stmt(stmt);
            }
            parser_destroy(parser);
        }
        free(sql);
    }

    /* Should parse all available tables */
    ASSERT_EQ(parsed_count, (int)table_count);

    TEST_PASS();
}

/* Test suite definition */
static TestCase parser_integration_tests[] = {
    {"parse_sakila_actor", test_parser_integration_parse_sakila_actor, "parser_integration"},
    {"parse_sakila_film", test_parser_integration_parse_sakila_film, "parser_integration"},
    {"parse_sakila_customer", test_parser_integration_parse_sakila_customer, "parser_integration"},
    {"parse_sakila_payment", test_parser_integration_parse_sakila_payment, "parser_integration"},
    {"parse_sakila_rental", test_parser_integration_parse_sakila_rental, "parser_integration"},
    {"parse_sakila_inventory", test_parser_integration_parse_sakila_inventory, "parser_integration"},
    {"parse_sakila_address", test_parser_integration_parse_sakila_address, "parser_integration"},
    {"parse_sakila_city", test_parser_integration_parse_sakila_city, "parser_integration"},
    {"parse_sakila_country", test_parser_integration_parse_sakila_country, "parser_integration"},
    {"parse_sakila_language", test_parser_integration_parse_sakila_language, "parser_integration"},
    {"parse_all_sakila_tables", test_parser_integration_parse_all_sakila_tables, "parser_integration"},
};

void run_parser_integration_tests(void) {
    run_test_suite("parser_integration", NULL, NULL, parser_integration_tests,
                   sizeof(parser_integration_tests) / sizeof(parser_integration_tests[0]));
}
