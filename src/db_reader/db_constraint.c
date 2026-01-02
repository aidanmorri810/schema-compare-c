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
                /* Parse column list from definition and add UNIQUE constraint to columns */
                /* Definition format: "UNIQUE (col1, col2, ...)" or "UNIQUE (col1, col2, ...) NULLS [NOT] DISTINCT" */
                {
                    /* Initialize the unique constraint */
                    constraint->constraint.unique.columns = NULL;
                    constraint->constraint.unique.column_count = 0;
                    constraint->constraint.unique.index_params = NULL;
                    constraint->constraint.unique.has_nulls_distinct = false;

                    const char *unique_start = strstr(condef, "UNIQUE (");
                    if (unique_start) {
                        unique_start += 8; /* Skip "UNIQUE (" */
                        const char *unique_end = strchr(unique_start, ')');
                        if (unique_end && unique_end > unique_start) {
                            size_t cols_len = unique_end - unique_start;
                            char *cols_str = malloc(cols_len + 1);
                            if (cols_str) {
                                memcpy(cols_str, unique_start, cols_len);
                                cols_str[cols_len] = '\0';

                                /* Count columns */
                                int col_count = 1;
                                for (size_t i = 0; i < cols_len; i++) {
                                    if (cols_str[i] == ',') col_count++;
                                }

                                /* Allocate column array */
                                constraint->constraint.unique.columns = mem_alloc(mem_ctx, sizeof(char*) * col_count);
                                if (constraint->constraint.unique.columns) {
                                    /* Parse column names and add UNIQUE constraint to each */
                                    char *saveptr = NULL;
                                    char *col_name = strtok_r(cols_str, ", ", &saveptr);
                                    int idx = 0;
                                    while (col_name && idx < col_count) {
                                        /* Trim whitespace */
                                        while (*col_name == ' ') col_name++;
                                        if (*col_name != '\0') {
                                            char *end = col_name + strlen(col_name) - 1;
                                            while (end > col_name && *end == ' ') {
                                                *end = '\0';
                                                end--;
                                            }
                                            constraint->constraint.unique.columns[idx++] = mem_strdup(mem_ctx, col_name);

                                            /* Add UNIQUE constraint to the column */
                                            if (stmt->table_def.regular.elements) {
                                                TableElement *elem = stmt->table_def.regular.elements;
                                                bool found = false;
                                                while (elem && !found) {
                                                    if (elem->type == TABLE_ELEM_COLUMN) {
                                                        if (strcmp(elem->elem.column.column_name, col_name) == 0) {
                                                            /* Add UNIQUE constraint to this column */
                                                            ColumnConstraint *unique_constraint = column_constraint_alloc(mem_ctx);
                                                            if (unique_constraint) {
                                                                unique_constraint->type = CONSTRAINT_UNIQUE;
                                                                unique_constraint->constraint_name = NULL;
                                                                unique_constraint->has_deferrable = false;
                                                                unique_constraint->has_initially = false;
                                                                unique_constraint->has_enforced = false;
                                                                unique_constraint->next = elem->elem.column.constraints;
                                                                elem->elem.column.constraints = unique_constraint;
                                                            }
                                                            found = true;
                                                        }
                                                    }
                                                    elem = elem->next;
                                                }
                                            }
                                        }
                                        col_name = strtok_r(NULL, ", ", &saveptr);
                                    }
                                    constraint->constraint.unique.column_count = idx;
                                }
                                free(cols_str);
                            }

                            /* Check for NULLS DISTINCT/NOT DISTINCT */
                            const char *nulls_pos = strstr(unique_end, "NULLS ");
                            if (nulls_pos) {
                                constraint->constraint.unique.has_nulls_distinct = true;
                                nulls_pos += 6; /* Skip "NULLS " */
                                if (strncmp(nulls_pos, "NOT DISTINCT", 12) == 0) {
                                    constraint->constraint.unique.nulls_distinct = NULLS_NOT_DISTINCT;
                                } else {
                                    constraint->constraint.unique.nulls_distinct = NULLS_DISTINCT;
                                }
                            }
                        }
                    }
                }
                break;

            case 'p': /* PRIMARY KEY constraint */
                constraint->type = TABLE_CONSTRAINT_PRIMARY_KEY;
                /* Parse column list from definition and add PRIMARY KEY constraint to columns */
                /* Definition format: "PRIMARY KEY (col1, col2, ...)" */
                {
                    const char *pk_start = strstr(condef, "PRIMARY KEY (");
                    if (pk_start) {
                        pk_start += 13; /* Skip "PRIMARY KEY (" */
                        const char *pk_end = strchr(pk_start, ')');
                        if (pk_end && pk_end > pk_start) {
                            size_t cols_len = pk_end - pk_start;
                            char *cols_str = malloc(cols_len + 1);
                            if (cols_str) {
                                memcpy(cols_str, pk_start, cols_len);
                                cols_str[cols_len] = '\0';

                                /* Parse comma-separated column list and add PK constraint to each */
                                char *saveptr = NULL;
                                char *col_name = strtok_r(cols_str, ", ", &saveptr);
                                while (col_name) {
                                    /* Trim whitespace from column name */
                                    while (*col_name == ' ') col_name++;

                                    /* Skip empty strings after trimming */
                                    if (*col_name == '\0') {
                                        col_name = strtok_r(NULL, ", ", &saveptr);
                                        continue;
                                    }

                                    char *end = col_name + strlen(col_name) - 1;
                                    while (end > col_name && *end == ' ') {
                                        *end = '\0';
                                        end--;
                                    }

                                    /* Find the column in the table elements */
                                    if (stmt->table_def.regular.elements) {
                                        TableElement *elem = stmt->table_def.regular.elements;
                                        bool found = false;
                                        while (elem && !found) {
                                            if (elem->type == TABLE_ELEM_COLUMN) {
                                                if (strcmp(elem->elem.column.column_name, col_name) == 0) {
                                                    /* Add PRIMARY KEY constraint to this column */
                                                    ColumnConstraint *pk_constraint = column_constraint_alloc(mem_ctx);
                                                    if (pk_constraint) {
                                                        pk_constraint->type = CONSTRAINT_PRIMARY_KEY;
                                                        pk_constraint->constraint_name = NULL;
                                                        pk_constraint->has_deferrable = false;
                                                        pk_constraint->has_initially = false;
                                                        pk_constraint->has_enforced = false;
                                                        pk_constraint->next = elem->elem.column.constraints;
                                                        elem->elem.column.constraints = pk_constraint;
                                                    }
                                                    found = true;
                                                }
                                            }
                                            elem = elem->next;
                                        }
                                    }
                                    col_name = strtok_r(NULL, ", ", &saveptr);
                                }
                                free(cols_str);
                            }
                        }
                    }
                }
                break;

            case 'f': /* FOREIGN KEY constraint */
                constraint->type = TABLE_CONSTRAINT_FOREIGN_KEY;
                /* Parse foreign key details from definition */
                /* Definition format: "FOREIGN KEY (cols) REFERENCES table(refcols) [ON DELETE action] [ON UPDATE action]" */
                {
                    /* Initialize the foreign key constraint */
                    constraint->constraint.foreign_key.columns = NULL;
                    constraint->constraint.foreign_key.column_count = 0;
                    constraint->constraint.foreign_key.reftable = NULL;
                    constraint->constraint.foreign_key.refcolumns = NULL;
                    constraint->constraint.foreign_key.refcolumn_count = 0;
                    constraint->constraint.foreign_key.has_match_type = false;
                    constraint->constraint.foreign_key.has_on_delete = false;
                    constraint->constraint.foreign_key.has_on_update = false;

                    /* Parse source columns: "FOREIGN KEY (col1, col2, ...)" */
                    const char *fk_start = strstr(condef, "FOREIGN KEY (");
                    if (fk_start) {
                        fk_start += 13; /* Skip "FOREIGN KEY (" */
                        const char *fk_end = strchr(fk_start, ')');
                        if (fk_end && fk_end > fk_start) {
                            size_t cols_len = fk_end - fk_start;
                            char *cols_str = malloc(cols_len + 1);
                            if (cols_str) {
                                memcpy(cols_str, fk_start, cols_len);
                                cols_str[cols_len] = '\0';

                                /* Count columns */
                                int col_count = 1;
                                for (size_t i = 0; i < cols_len; i++) {
                                    if (cols_str[i] == ',') col_count++;
                                }

                                /* Allocate column array */
                                constraint->constraint.foreign_key.columns = mem_alloc(mem_ctx, sizeof(char*) * col_count);
                                if (constraint->constraint.foreign_key.columns) {
                                    /* Parse column names */
                                    char *saveptr = NULL;
                                    char *col_name = strtok_r(cols_str, ", ", &saveptr);
                                    int idx = 0;
                                    while (col_name && idx < col_count) {
                                        /* Trim whitespace */
                                        while (*col_name == ' ') col_name++;
                                        if (*col_name != '\0') {
                                            char *end = col_name + strlen(col_name) - 1;
                                            while (end > col_name && *end == ' ') {
                                                *end = '\0';
                                                end--;
                                            }
                                            constraint->constraint.foreign_key.columns[idx++] = mem_strdup(mem_ctx, col_name);
                                        }
                                        col_name = strtok_r(NULL, ", ", &saveptr);
                                    }
                                    constraint->constraint.foreign_key.column_count = idx;
                                }
                                free(cols_str);
                            }
                        }
                    }

                    /* Parse referenced table and columns: "REFERENCES table(col1, col2, ...)" */
                    const char *ref_start = strstr(condef, "REFERENCES ");
                    if (ref_start) {
                        ref_start += 11; /* Skip "REFERENCES " */
                        const char *ref_paren = strchr(ref_start, '(');
                        if (ref_paren) {
                            /* Extract table name */
                            size_t table_len = ref_paren - ref_start;
                            char *table_str = malloc(table_len + 1);
                            if (table_str) {
                                memcpy(table_str, ref_start, table_len);
                                table_str[table_len] = '\0';
                                /* Trim whitespace from table name */
                                char *table_name = table_str;
                                while (*table_name == ' ') table_name++;
                                char *end = table_name + strlen(table_name) - 1;
                                while (end > table_name && *end == ' ') {
                                    *end = '\0';
                                    end--;
                                }
                                constraint->constraint.foreign_key.reftable = mem_strdup(mem_ctx, table_name);
                                free(table_str);
                            }

                            /* Parse referenced columns */
                            const char *refcol_start = ref_paren + 1;
                            const char *refcol_end = strchr(refcol_start, ')');
                            if (refcol_end && refcol_end > refcol_start) {
                                size_t refcols_len = refcol_end - refcol_start;
                                char *refcols_str = malloc(refcols_len + 1);
                                if (refcols_str) {
                                    memcpy(refcols_str, refcol_start, refcols_len);
                                    refcols_str[refcols_len] = '\0';

                                    /* Count referenced columns */
                                    int refcol_count = 1;
                                    for (size_t i = 0; i < refcols_len; i++) {
                                        if (refcols_str[i] == ',') refcol_count++;
                                    }

                                    /* Allocate refcolumn array */
                                    constraint->constraint.foreign_key.refcolumns = mem_alloc(mem_ctx, sizeof(char*) * refcol_count);
                                    if (constraint->constraint.foreign_key.refcolumns) {
                                        /* Parse refcolumn names */
                                        char *saveptr = NULL;
                                        char *refcol_name = strtok_r(refcols_str, ", ", &saveptr);
                                        int idx = 0;
                                        while (refcol_name && idx < refcol_count) {
                                            /* Trim whitespace */
                                            while (*refcol_name == ' ') refcol_name++;
                                            if (*refcol_name != '\0') {
                                                char *end = refcol_name + strlen(refcol_name) - 1;
                                                while (end > refcol_name && *end == ' ') {
                                                    *end = '\0';
                                                    end--;
                                                }
                                                constraint->constraint.foreign_key.refcolumns[idx++] = mem_strdup(mem_ctx, refcol_name);
                                            }
                                            refcol_name = strtok_r(NULL, ", ", &saveptr);
                                        }
                                        constraint->constraint.foreign_key.refcolumn_count = idx;
                                    }
                                    free(refcols_str);
                                }
                            }
                        }
                    }

                    /* Parse ON DELETE action */
                    const char *on_delete = strstr(condef, "ON DELETE ");
                    if (on_delete) {
                        on_delete += 10; /* Skip "ON DELETE " */
                        constraint->constraint.foreign_key.has_on_delete = true;
                        if (strncmp(on_delete, "CASCADE", 7) == 0) {
                            constraint->constraint.foreign_key.on_delete = REF_ACTION_CASCADE;
                        } else if (strncmp(on_delete, "SET NULL", 8) == 0) {
                            constraint->constraint.foreign_key.on_delete = REF_ACTION_SET_NULL;
                        } else if (strncmp(on_delete, "SET DEFAULT", 11) == 0) {
                            constraint->constraint.foreign_key.on_delete = REF_ACTION_SET_DEFAULT;
                        } else if (strncmp(on_delete, "RESTRICT", 8) == 0) {
                            constraint->constraint.foreign_key.on_delete = REF_ACTION_RESTRICT;
                        } else {
                            constraint->constraint.foreign_key.on_delete = REF_ACTION_NO_ACTION;
                        }
                    }

                    /* Parse ON UPDATE action */
                    const char *on_update = strstr(condef, "ON UPDATE ");
                    if (on_update) {
                        on_update += 10; /* Skip "ON UPDATE " */
                        constraint->constraint.foreign_key.has_on_update = true;
                        if (strncmp(on_update, "CASCADE", 7) == 0) {
                            constraint->constraint.foreign_key.on_update = REF_ACTION_CASCADE;
                        } else if (strncmp(on_update, "SET NULL", 8) == 0) {
                            constraint->constraint.foreign_key.on_update = REF_ACTION_SET_NULL;
                        } else if (strncmp(on_update, "SET DEFAULT", 11) == 0) {
                            constraint->constraint.foreign_key.on_update = REF_ACTION_SET_DEFAULT;
                        } else if (strncmp(on_update, "RESTRICT", 8) == 0) {
                            constraint->constraint.foreign_key.on_update = REF_ACTION_RESTRICT;
                        } else {
                            constraint->constraint.foreign_key.on_update = REF_ACTION_NO_ACTION;
                        }
                    }
                }
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
