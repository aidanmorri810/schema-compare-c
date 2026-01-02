#include "db_reader.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Read single table from database */
CreateTableStmt *db_read_table(DBConnection *conn, const char *schema,
                               const char *table_name, MemoryContext *mem_ctx) {
    if (!conn || !db_is_connected(conn) || !table_name) {
        return NULL;
    }

    /* Use public schema if not specified */
    if (!schema) {
        schema = "public";
    }

    log_info("Reading table: %s.%s", schema, table_name);

    /* Create statement */
    CreateTableStmt *stmt = create_table_stmt_alloc(mem_ctx);
    if (!stmt) {
        log_error("Failed to allocate CreateTableStmt");
        return NULL;
    }

    /* Initialize */
    stmt->variant = CREATE_TABLE_REGULAR;
    stmt->table_name = mem_strdup(mem_ctx, table_name);
    stmt->temp_scope = TEMP_SCOPE_NONE;
    stmt->table_type = TABLE_TYPE_NORMAL;
    stmt->if_not_exists = false;

    /* Populate table info */
    if (!db_populate_table_info(conn, schema, table_name, stmt, mem_ctx)) {
        log_error("Failed to populate table info for %s.%s", schema, table_name);
        return NULL;
    }

    /* Populate columns */
    if (!db_populate_columns(conn, schema, table_name, stmt, mem_ctx)) {
        log_error("Failed to populate columns for %s.%s", schema, table_name);
        return NULL;
    }

    /* Populate constraints */
    if (!db_populate_constraints(conn, schema, table_name, stmt, mem_ctx)) {
        log_error("Failed to populate constraints for %s.%s", schema, table_name);
        return NULL;
    }

    log_info("Successfully read table: %s.%s", schema, table_name);
    return stmt;
}

/* Read all tables from a schema */
CreateTableStmt **db_read_schema(DBConnection *conn, const char *schema_name,
                                 int *table_count, MemoryContext *mem_ctx) {
    if (!conn || !db_is_connected(conn) || !table_count) {
        return NULL;
    }

    if (!schema_name) {
        schema_name = "public";
    }

    *table_count = 0;

    /* Query to get all tables in schema */
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT tablename FROM pg_tables "
             "WHERE schemaname = '%s' "
             "ORDER BY tablename",
             schema_name);

    PGresult *res = PQexec(conn->conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        log_error("Failed to query tables: %s", PQerrorMessage(conn->conn));
        PQclear(res);
        return NULL;
    }

    int nrows = PQntuples(res);
    if (nrows == 0) {
        PQclear(res);
        return NULL;
    }

    /* Allocate array for table statements */
    CreateTableStmt **tables = mem_alloc(mem_ctx, sizeof(CreateTableStmt *) * nrows);
    if (!tables) {
        PQclear(res);
        return NULL;
    }

    /* Read each table */
    int count = 0;
    for (int i = 0; i < nrows; i++) {
        const char *table_name = PQgetvalue(res, i, 0);
        CreateTableStmt *stmt = db_read_table(conn, schema_name, table_name, mem_ctx);
        if (stmt) {
            tables[count++] = stmt;
        }
    }

    PQclear(res);
    *table_count = count;
    return tables;
}

/* Read tables with options */
CreateTableStmt **db_read_all_tables(DBConnection *conn,
                                     const IntrospectionOptions *opts,
                                     int *table_count, MemoryContext *mem_ctx) {
    if (!conn || !db_is_connected(conn) || !table_count) {
        return NULL;
    }

    *table_count = 0;

    /* If specific schemas provided, read those */
    if (opts && opts->schemas && opts->schema_count > 0) {
        int total_count = 0;
        int capacity = 100;
        CreateTableStmt **all_tables = mem_alloc(mem_ctx, sizeof(CreateTableStmt *) * capacity);
        if (!all_tables) {
            return NULL;
        }

        for (int i = 0; i < opts->schema_count; i++) {
            int schema_count = 0;
            CreateTableStmt **schema_tables = db_read_schema(conn, opts->schemas[i],
                                                            &schema_count, mem_ctx);
            if (schema_tables) {
                for (int j = 0; j < schema_count; j++) {
                    if (total_count >= capacity) {
                        capacity *= 2;
                        CreateTableStmt **new_tables = mem_alloc(mem_ctx,
                                                                sizeof(CreateTableStmt *) * capacity);
                        if (!new_tables) {
                            *table_count = total_count;
                            return all_tables;
                        }
                        memcpy(new_tables, all_tables, sizeof(CreateTableStmt *) * total_count);
                        all_tables = new_tables;
                    }
                    all_tables[total_count++] = schema_tables[j];
                }
            }
        }

        *table_count = total_count;
        return all_tables;
    }

    /* Otherwise read from public schema */
    return db_read_schema(conn, "public", table_count, mem_ctx);
}
