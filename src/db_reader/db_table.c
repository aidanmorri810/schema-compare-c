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
