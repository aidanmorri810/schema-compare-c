#include "db_reader.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Populate columns for multiple tables in a single batch query */
bool db_populate_columns(DBConnection *conn, const char *schema,
                        CreateTableStmt **stmts, int stmt_count,
                        MemoryContext *mem_ctx) {
    if (!conn || !stmts || stmt_count <= 0) {
        return false;
    }

    /* Build query with all table names */
    char query[8192];
    int offset = snprintf(query, sizeof(query),
             "SELECT "
             "  c.relname, "                    /* table name */
             "  a.attname, "                    /* column name */
             "  pg_catalog.format_type(a.atttypid, a.atttypmod), " /* data type */
             "  a.attnotnull, "                 /* NOT NULL */
             "  pg_get_expr(d.adbin, d.adrelid), " /* DEFAULT value */
             "  a.attidentity, "                /* GENERATED identity */
             "  a.attgenerated, "               /* GENERATED column */
             "  col.collname, "                 /* COLLATE */
             "  a.attstorage "                  /* STORAGE */
             "FROM pg_attribute a "
             "JOIN pg_class c ON a.attrelid = c.oid "
             "JOIN pg_namespace n ON c.relnamespace = n.oid "
             "LEFT JOIN pg_attrdef d ON a.attrelid = d.adrelid AND a.attnum = d.adnum "
             "LEFT JOIN pg_collation col ON a.attcollation = col.oid AND a.attcollation <> 0 "
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

    offset += snprintf(query + offset, sizeof(query) - offset,
             ") "
             "  AND a.attnum > 0 "
             "  AND NOT a.attisdropped "
             "ORDER BY c.relname, a.attnum");

    PGresult *res = PQexec(conn->conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        log_error("Failed to query columns in batch: %s", PQerrorMessage(conn->conn));
        PQclear(res);
        return false;
    }

    int nrows = PQntuples(res);
    if (nrows == 0) {
        log_warn("No columns found for %d tables in schema %s", stmt_count, schema);
        PQclear(res);
        return true;
    }

    /* Process results and organize by table */
    const char *current_table = NULL;
    CreateTableStmt *current_stmt = NULL;
    TableElement *head = NULL;
    TableElement *tail = NULL;

    for (int i = 0; i < nrows; i++) {
        const char *table_name = PQgetvalue(res, i, 0);

        /* Check if we've moved to a new table */
        if (!current_table || strcmp(current_table, table_name) != 0) {
            /* Save previous table's columns */
            if (current_stmt) {
                current_stmt->table_def.regular.elements = head;
            }

            /* Find the statement for this table */
            current_table = table_name;
            current_stmt = NULL;
            for (int j = 0; j < stmt_count; j++) {
                if (strcmp(stmts[j]->table_name, table_name) == 0) {
                    current_stmt = stmts[j];
                    break;
                }
            }

            if (!current_stmt) {
                log_error("Could not find statement for table %s", table_name);
                PQclear(res);
                return false;
            }

            /* Reset list for new table */
            head = NULL;
            tail = NULL;
        }

        /* Allocate table element */
        TableElement *elem = table_element_alloc(mem_ctx);
        if (!elem) {
            PQclear(res);
            return false;
        }

        elem->type = TABLE_ELEM_COLUMN;
        elem->next = NULL;

        /* Allocate column definition */
        ColumnDef *col = column_def_alloc(mem_ctx);
        if (!col) {
            PQclear(res);
            return false;
        }

        /* Column name (index 1 now because table name is first) */
        col->column_name = mem_strdup(mem_ctx, PQgetvalue(res, i, 1));

        /* Data type */
        col->data_type = mem_strdup(mem_ctx, PQgetvalue(res, i, 2));

        /* Collation */
        if (!PQgetisnull(res, i, 7)) {
            col->collation = mem_strdup(mem_ctx, PQgetvalue(res, i, 7));
        } else {
            col->collation = NULL;
        }

        /* Storage type */
        if (!PQgetisnull(res, i, 8)) {
            const char *storage = PQgetvalue(res, i, 8);
            col->has_storage = true;
            switch (storage[0]) {
                case 'p': col->storage_type = STORAGE_TYPE_PLAIN; break;
                case 'e': col->storage_type = STORAGE_TYPE_EXTERNAL; break;
                case 'x': col->storage_type = STORAGE_TYPE_EXTENDED; break;
                case 'm': col->storage_type = STORAGE_TYPE_MAIN; break;
                default: col->has_storage = false;
            }
        } else {
            col->has_storage = false;
        }

        col->compression_method = NULL;
        col->constraints = NULL;

        /* Handle NOT NULL constraint */
        const char *notnull = PQgetvalue(res, i, 3);
        if (strcmp(notnull, "t") == 0) {
            ColumnConstraint *constraint = column_constraint_alloc(mem_ctx);
            if (constraint) {
                constraint->type = CONSTRAINT_NOT_NULL;
                constraint->constraint_name = NULL;
                constraint->constraint.not_null.no_inherit = false;
                constraint->has_deferrable = false;
                constraint->has_initially = false;
                constraint->has_enforced = false;
                constraint->next = col->constraints;
                col->constraints = constraint;
            }
        }

        /* Handle DEFAULT value */
        if (!PQgetisnull(res, i, 4)) {
            const char *default_expr = PQgetvalue(res, i, 4);
            ColumnConstraint *constraint = column_constraint_alloc(mem_ctx);
            if (constraint) {
                constraint->type = CONSTRAINT_DEFAULT;
                constraint->constraint_name = NULL;
                constraint->constraint.default_val.expr = expression_alloc(mem_ctx, default_expr);
                constraint->has_deferrable = false;
                constraint->has_initially = false;
                constraint->has_enforced = false;
                constraint->next = col->constraints;
                col->constraints = constraint;
            }
        }

        /* Handle GENERATED identity */
        if (!PQgetisnull(res, i, 5)) {
            const char *identity = PQgetvalue(res, i, 5);
            if (identity[0] != '\0') {
                ColumnConstraint *constraint = column_constraint_alloc(mem_ctx);
                if (constraint) {
                    constraint->type = CONSTRAINT_GENERATED_IDENTITY;
                    constraint->constraint_name = NULL;
                    constraint->constraint.generated_identity.type =
                        (identity[0] == 'a') ? IDENTITY_ALWAYS : IDENTITY_BY_DEFAULT;
                    constraint->constraint.generated_identity.sequence_opts = NULL;
                    constraint->has_deferrable = false;
                    constraint->has_initially = false;
                    constraint->has_enforced = false;
                    constraint->next = col->constraints;
                    col->constraints = constraint;
                }
            }
        }

        /* Handle GENERATED column */
        if (!PQgetisnull(res, i, 6)) {
            const char *generated = PQgetvalue(res, i, 6);
            if (generated[0] == 's') {
                /* Generated stored column */
                log_warn("GENERATED column detected but expression extraction not implemented");
            }
        }

        elem->elem.column = *col;

        /* Add to list */
        if (!head) {
            head = elem;
            tail = elem;
        } else {
            tail->next = elem;
            tail = elem;
        }
    }

    /* Save the last table's columns */
    if (current_stmt) {
        current_stmt->table_def.regular.elements = head;
    }

    PQclear(res);
    return true;
}
