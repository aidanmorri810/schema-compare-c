#include "db_reader.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Populate basic table information for multiple tables in a single batch query */
bool db_populate_table_info(DBConnection *conn, const char *schema,
                            CreateTableStmt **stmts, int stmt_count,
                            MemoryContext *mem_ctx) {
    if (!conn || !stmts || stmt_count <= 0) {
        return false;
    }

    /* Build query with all table names */
    char query[4096];
    int offset = snprintf(query, sizeof(query),
             "SELECT "
             "  c.relname, "          /* table name */
             "  c.relpersistence, "  /* t=temp, u=unlogged, p=permanent */
             "  c.relkind, "          /* r=ordinary table, p=partitioned table */
             "  ts.spcname "          /* tablespace */
             "FROM pg_class c "
             "JOIN pg_namespace n ON c.relnamespace = n.oid "
             "LEFT JOIN pg_tablespace ts ON c.reltablespace = ts.oid "
             "WHERE n.nspname = '%s' "
             "  AND c.relname IN (", schema);

    /* Add table names */
    for (int i = 0; i < stmt_count; i++) {
        if (i > 0) {
            offset += snprintf(query + offset, sizeof(query) - offset, ", ");
        }
        offset += snprintf(query + offset, sizeof(query) - offset, "'%s'",
                          stmts[i]->table_name);
    }

    offset += snprintf(query + offset, sizeof(query) - offset, ")");

    PGresult *res = PQexec(conn->conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        log_error("Failed to query table info in batch: %s", PQerrorMessage(conn->conn));
        PQclear(res);
        return false;
    }

    int nrows = PQntuples(res);
    if (nrows == 0) {
        log_error("No tables found");
        PQclear(res);
        return false;
    }

    /* Process results and match to statements */
    for (int i = 0; i < nrows; i++) {
        const char *table_name = PQgetvalue(res, i, 0);

        /* Find the statement for this table */
        CreateTableStmt *stmt = NULL;
        for (int j = 0; j < stmt_count; j++) {
            if (strcmp(stmts[j]->table_name, table_name) == 0) {
                stmt = stmts[j];
                break;
            }
        }

        if (!stmt) {
            log_error("Could not find statement for table %s", table_name);
            PQclear(res);
            return false;
        }

        /* Parse relpersistence */
        const char *persistence = PQgetvalue(res, i, 1);
        if (strcmp(persistence, "t") == 0) {
            stmt->table_type = TABLE_TYPE_TEMPORARY;
        } else if (strcmp(persistence, "u") == 0) {
            stmt->table_type = TABLE_TYPE_UNLOGGED;
        } else {
            stmt->table_type = TABLE_TYPE_NORMAL;
        }

        /* Parse relkind */
        const char *relkind = PQgetvalue(res, i, 2);
        if (strcmp(relkind, "p") == 0) {
            /* Partitioned table - TODO: handle partitioning */
            log_warn("Table %s.%s is partitioned - partition info not yet implemented",
                     schema, table_name);
        }

        /* Parse tablespace */
        if (!PQgetisnull(res, i, 3)) {
            const char *tablespace = PQgetvalue(res, i, 3);
            stmt->tablespace_name = mem_strdup(mem_ctx, tablespace);
        } else {
            stmt->tablespace_name = NULL;
        }
    }

    PQclear(res);
    return true;
}

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

    /* Create array for batch functions (single table) */
    CreateTableStmt *stmts[1] = { stmt };

    /* Populate table info using batch function */
    if (!db_populate_table_info(conn, schema, stmts, 1, mem_ctx)) {
        log_error("Failed to populate table info for %s.%s", schema, table_name);
        return NULL;
    }

    /* Populate columns using batch function */
    if (!db_populate_columns(conn, schema, stmts, 1, mem_ctx)) {
        log_error("Failed to populate columns for %s.%s", schema, table_name);
        return NULL;
    }

    /* Populate constraints using batch function */
    if (!db_populate_constraints(conn, schema, stmts, 1, mem_ctx)) {
        log_error("Failed to populate constraints for %s.%s", schema, table_name);
        return NULL;
    }

    log_info("Successfully read table: %s.%s", schema, table_name);
    return stmt;
}

/* Read all tables from a schema */
CreateTableStmt **db_read_all_tables(DBConnection *conn, const char *schema_name,
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

    /* Create statements for all tables */
    int count = 0;
    for (int i = 0; i < nrows; i++) {
        const char *table_name = PQgetvalue(res, i, 0);

        log_info("Reading table: %s.%s", schema_name, table_name);

        /* Create statement */
        CreateTableStmt *stmt = create_table_stmt_alloc(mem_ctx);
        if (!stmt) {
            log_error("Failed to allocate CreateTableStmt");
            continue;
        }

        /* Initialize */
        stmt->variant = CREATE_TABLE_REGULAR;
        stmt->table_name = mem_strdup(mem_ctx, table_name);
        stmt->temp_scope = TEMP_SCOPE_NONE;
        stmt->table_type = TABLE_TYPE_NORMAL;
        stmt->if_not_exists = false;

        tables[count++] = stmt;
    }

    PQclear(res);

    if (count == 0) {
        *table_count = 0;
        return NULL;
    }

    /* Batch populate table info for all tables in one query */
    if (!db_populate_table_info(conn, schema_name, tables, count, mem_ctx)) {
        log_error("Failed to batch populate table info for schema %s", schema_name);
        *table_count = 0;
        return NULL;
    }

    /* Batch populate columns for all tables in one query */
    if (!db_populate_columns(conn, schema_name, tables, count, mem_ctx)) {
        log_error("Failed to batch populate columns for schema %s", schema_name);
        *table_count = 0;
        return NULL;
    }

    /* Batch populate constraints for all tables in one query */
    if (!db_populate_constraints(conn, schema_name, tables, count, mem_ctx)) {
        log_error("Failed to batch populate constraints for schema %s", schema_name);
        *table_count = 0;
        return NULL;
    }

    /* Log success for all tables */
    for (int i = 0; i < count; i++) {
        log_info("Successfully read table: %s.%s", schema_name, tables[i]->table_name);
    }

    *table_count = count;
    return tables;
}