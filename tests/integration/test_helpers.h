#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include "pg_create_table.h"
#include "db_reader.h"
#include "sc_memory.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <libpq-fe.h>

/**
 * Find a column by name in the table elements linked list.
 * Returns NULL if not found.
 */
__attribute__((unused))
static ColumnDef* find_column(TableElement *elements, const char *column_name) {
    TableElement *current = elements;
    while (current != NULL) {
        if (current->type == TABLE_ELEM_COLUMN) {
            if (strcmp(current->elem.column.column_name, column_name) == 0) {
                return &current->elem.column;
            }
        }
        current = current->next;
    }
    return NULL;
}

/**
 * Find a column constraint of a specific type in a column's constraint list.
 * Returns NULL if not found.
 */
__attribute__((unused))
static ColumnConstraint* find_column_constraint(ColumnConstraint *constraints, ConstraintType type) {
    ColumnConstraint *current = constraints;
    while (current != NULL) {
        if (current->type == type) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

/**
 * Find a table constraint by name in the table elements linked list.
 * Returns NULL if not found.
 */
__attribute__((unused))
static TableConstraint* find_table_constraint(TableElement *elements, const char *constraint_name) {
    TableElement *current = elements;
    while (current != NULL) {
        if (current->type == TABLE_ELEM_TABLE_CONSTRAINT) {
            TableConstraint *tc = current->elem.table_constraint;
            if (tc->constraint_name && strcmp(tc->constraint_name, constraint_name) == 0) {
                return tc;
            }
        }
        current = current->next;
    }
    return NULL;
}

/**
 * Count total number of columns in the table.
 */
__attribute__((unused))
static int count_columns(TableElement *elements) {
    int count = 0;
    TableElement *current = elements;
    while (current != NULL) {
        if (current->type == TABLE_ELEM_COLUMN) {
            count++;
        }
        current = current->next;
    }
    return count;
}

/**
 * Check if a column has a specific constraint type.
 */
__attribute__((unused))
static bool column_has_constraint(ColumnDef *col, ConstraintType type) {
    return find_column_constraint(col->constraints, type) != NULL;
}

/**
 * Execute SQL and check for errors.
 */
__attribute__((unused))
static void execute_sql_helper(DBConnection *conn, const char *sql) {
    if (!conn || !sql) return;

    PGresult *res = PQexec(conn->conn, sql);
    if (PQresultStatus(res) != PGRES_COMMAND_OK &&
        PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SQL execution failed: %s\n", PQerrorMessage(conn->conn));
    }
    PQclear(res);
}

/**
 * Apply SQL file to database.
 * Reads SQL file and executes it.
 */
__attribute__((unused))
static bool apply_sql_file_to_db(DBConnection *conn, const char *sql_file_path) {
    if (!conn || !sql_file_path) return false;

    /* Read SQL file */
    FILE *file = fopen(sql_file_path, "r");
    if (!file) {
        fprintf(stderr, "Failed to open SQL file: %s\n", sql_file_path);
        return false;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *sql = malloc(file_size + 1);
    if (!sql) {
        fclose(file);
        return false;
    }

    size_t bytes_read = fread(sql, 1, file_size, file);
    sql[bytes_read] = '\0';
    fclose(file);

    /* Execute SQL */
    PGresult *res = PQexec(conn->conn, sql);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK ||
                   PQresultStatus(res) == PGRES_TUPLES_OK);

    if (!success) {
        fprintf(stderr, "Failed to execute SQL from %s: %s\n",
                sql_file_path, PQerrorMessage(conn->conn));
    }

    PQclear(res);
    free(sql);
    return success;
}

/**
 * Drop table from database if it exists.
 */
__attribute__((unused))
static void drop_table_if_exists(DBConnection *conn, const char *table_name) {
    if (!conn || !table_name) return;

    char sql[256];
    snprintf(sql, sizeof(sql), "DROP TABLE IF EXISTS %s CASCADE;", table_name);
    execute_sql_helper(conn, sql);
}

/**
 * Setup database connection for tests.
 * Returns NULL if database is not available.
 */
__attribute__((unused))
static DBConnection* setup_test_db_connection(DBConfig *config) {
    /* Check if database tests are enabled */
    const char *run_db_tests = getenv("RUN_DB_TESTS");
    if (!run_db_tests || strcmp(run_db_tests, "1") != 0) {
        return NULL;
    }

    DBConnection *conn = db_connect(config);
    if (conn && db_is_connected(conn)) {
        /* Suppress NOTICE messages for cleaner test output */
        execute_sql_helper(conn, "SET client_min_messages = WARNING;");
        return conn;
    }

    return NULL;
}

/**
 * Get global test database configuration.
 */
__attribute__((unused))
static DBConfig get_test_db_config(void) {
    DBConfig config;
    config.host = getenv("PGHOST") ? getenv("PGHOST") : "localhost";
    config.port = getenv("PGPORT") ? getenv("PGPORT") : "5433";
    config.database = getenv("PGDATABASE") ? getenv("PGDATABASE") : "schema_compare_test";
    config.user = getenv("PGUSER") ? getenv("PGUSER") : "testuser";
    config.password = getenv("PGPASSWORD") ? getenv("PGPASSWORD") : "testpass";
    config.connect_timeout = 10;
    return config;
}

#endif /* TEST_HELPERS_H */
