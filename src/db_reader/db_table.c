#include "db_reader.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Populate basic table information */
bool db_populate_table_info(DBConnection *conn, const char *schema,
                            const char *table_name, CreateTableStmt *stmt,
                            MemoryContext *mem_ctx) {
    if (!conn || !table_name || !stmt) {
        return false;
    }

    /* Query table metadata from pg_class */
    char query[1024];
    snprintf(query, sizeof(query),
             "SELECT "
             "  c.relpersistence, "  /* t=temp, u=unlogged, p=permanent */
             "  c.relkind, "          /* r=ordinary table, p=partitioned table */
             "  ts.spcname "          /* tablespace */
             "FROM pg_class c "
             "JOIN pg_namespace n ON c.relnamespace = n.oid "
             "LEFT JOIN pg_tablespace ts ON c.reltablespace = ts.oid "
             "WHERE n.nspname = '%s' AND c.relname = '%s'",
             schema, table_name);

    PGresult *res = PQexec(conn->conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        log_error("Failed to query table info: %s", PQerrorMessage(conn->conn));
        PQclear(res);
        return false;
    }

    if (PQntuples(res) == 0) {
        log_error("Table %s.%s not found", schema, table_name);
        PQclear(res);
        return false;
    }

    /* Parse relpersistence */
    const char *persistence = PQgetvalue(res, 0, 0);
    if (strcmp(persistence, "t") == 0) {
        stmt->table_type = TABLE_TYPE_TEMPORARY;
    } else if (strcmp(persistence, "u") == 0) {
        stmt->table_type = TABLE_TYPE_UNLOGGED;
    } else {
        stmt->table_type = TABLE_TYPE_NORMAL;
    }

    /* Parse relkind */
    const char *relkind = PQgetvalue(res, 0, 1);
    if (strcmp(relkind, "p") == 0) {
        /* Partitioned table - TODO: handle partitioning */
        log_warn("Table %s.%s is partitioned - partition info not yet implemented",
                 schema, table_name);
    }

    /* Parse tablespace */
    if (!PQgetisnull(res, 0, 2)) {
        const char *tablespace = PQgetvalue(res, 0, 2);
        stmt->tablespace_name = mem_strdup(mem_ctx, tablespace);
    } else {
        stmt->tablespace_name = NULL;
    }

    PQclear(res);
    return true;
}
