#include "db_reader.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Populate constraints from database */
bool db_populate_constraints(DBConnection *conn, const char *schema,
                            const char *table_name, CreateTableStmt *stmt,
                            MemoryContext *mem_ctx) {
    if (!conn || !table_name || !stmt) {
        return false;
    }

    /* Query constraints (CHECK, UNIQUE, PRIMARY KEY, FOREIGN KEY) */
    char query[2048];
    snprintf(query, sizeof(query),
             "SELECT "
             "  con.conname, "               /* constraint name */
             "  con.contype, "               /* constraint type */
             "  pg_get_constraintdef(con.oid), " /* constraint definition */
             "  con.condeferrable, "         /* deferrable */
             "  con.condeferred "            /* initially deferred */
             "FROM pg_constraint con "
             "JOIN pg_class c ON con.conrelid = c.oid "
             "JOIN pg_namespace n ON c.relnamespace = n.oid "
             "WHERE n.nspname = '%s' "
             "  AND c.relname = '%s' "
             "ORDER BY con.conname",
             schema, table_name);

    PGresult *res = PQexec(conn->conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        log_error("Failed to query constraints: %s", PQerrorMessage(conn->conn));
        PQclear(res);
        return false;
    }

    int nrows = PQntuples(res);
    if (nrows == 0) {
        /* No constraints - this is OK */
        PQclear(res);
        return true;
    }

    /* Find end of column list */
    TableElement *tail = stmt->table_def.regular.elements;
    while (tail && tail->next) {
        tail = tail->next;
    }

    for (int i = 0; i < nrows; i++) {
        const char *conname = PQgetvalue(res, i, 0);
        const char *contype = PQgetvalue(res, i, 1);
        const char *condef = PQgetvalue(res, i, 2);
        const char *condeferrable = PQgetvalue(res, i, 3);
        const char *condeferred = PQgetvalue(res, i, 4);

        /* Create table constraint */
        TableConstraint *constraint = table_constraint_alloc(mem_ctx);
        if (!constraint) {
            PQclear(res);
            return false;
        }

        constraint->constraint_name = mem_strdup(mem_ctx, conname);
        constraint->next = NULL;

        /* Set deferrable flags */
        constraint->has_deferrable = true;
        constraint->deferrable = (strcmp(condeferrable, "t") == 0);
        constraint->not_deferrable = !constraint->deferrable;

        constraint->has_initially = true;
        constraint->initially_deferred = (strcmp(condeferred, "t") == 0);
        constraint->initially_immediate = !constraint->initially_deferred;

        constraint->has_enforced = false;

        /* Parse constraint type */
        switch (contype[0]) {
            case 'c': /* CHECK constraint */
                constraint->type = TABLE_CONSTRAINT_CHECK;
                /* Extract CHECK expression from definition */
                /* Definition format: "CHECK (expression)" */
                {
                    const char *check_start = strstr(condef, "CHECK (");
                    if (check_start) {
                        check_start += 7; /* Skip "CHECK (" */
                        const char *check_end = strrchr(check_start, ')');
                        if (check_end) {
                            size_t expr_len = check_end - check_start;
                            char *expr_str = mem_alloc(mem_ctx, expr_len + 1);
                            if (expr_str) {
                                memcpy(expr_str, check_start, expr_len);
                                expr_str[expr_len] = '\0';
                                constraint->constraint.check.expr = expression_alloc(mem_ctx, expr_str);
                                constraint->constraint.check.no_inherit = false;
                            }
                        }
                    }
                }
                break;

            case 'u': /* UNIQUE constraint */
                constraint->type = TABLE_CONSTRAINT_UNIQUE;
                /* TODO: Parse column list from definition */
                log_warn("UNIQUE constraint column list parsing not implemented: %s", conname);
                break;

            case 'p': /* PRIMARY KEY constraint */
                constraint->type = TABLE_CONSTRAINT_PRIMARY_KEY;
                /* TODO: Parse column list from definition */
                log_warn("PRIMARY KEY constraint column list parsing not implemented: %s", conname);
                break;

            case 'f': /* FOREIGN KEY constraint */
                constraint->type = TABLE_CONSTRAINT_FOREIGN_KEY;
                /* TODO: Parse foreign key details from definition */
                log_warn("FOREIGN KEY constraint parsing not implemented: %s", conname);
                break;

            case 'x': /* EXCLUDE constraint */
                constraint->type = TABLE_CONSTRAINT_EXCLUDE;
                log_warn("EXCLUDE constraint parsing not implemented: %s", conname);
                break;

            default:
                log_warn("Unknown constraint type '%c' for %s", contype[0], conname);
                continue;
        }

        /* Add constraint as table element */
        TableElement *elem = table_element_alloc(mem_ctx);
        if (!elem) {
            PQclear(res);
            return false;
        }

        elem->type = TABLE_ELEM_TABLE_CONSTRAINT;
        elem->elem.table_constraint = constraint;
        elem->next = NULL;

        if (tail) {
            tail->next = elem;
            tail = elem;
        } else {
            stmt->table_def.regular.elements = elem;
            tail = elem;
        }
    }

    PQclear(res);
    return true;
}
