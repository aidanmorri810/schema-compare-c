#include "sql_generator.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

/* Forward declarations for constraint helpers (defined in sql_generator_constraint.c) */
char *generate_column_constraint(const ColumnConstraint *cc);
char *generate_table_constraint(const TableConstraint *tc);
char *generate_references_clause(const ColumnConstraint *cc);

/* Internal function to generate CREATE TABLE SQL with option to skip foreign keys (used by sql_generator.c) */
char *generate_create_table_sql_internal(const CreateTableStmt *stmt, const SQLGenOptions *opts, bool skip_foreign_keys) {
    if (!stmt || !stmt->table_name) {
        return NULL;
    }

    StringBuilder *sb = sb_create();
    if (!sb) {
        return NULL;
    }

    if (opts->add_comments) {
        sb_append(sb, "-- Create table ");
        sb_append(sb, stmt->table_name);
        sb_append(sb, "\n");
    }

    sb_append(sb, "CREATE ");

    /* Table type modifiers */
    if (stmt->table_type == TABLE_TYPE_TEMPORARY || stmt->table_type == TABLE_TYPE_TEMP) {
        sb_append(sb, "TEMPORARY ");
    } else if (stmt->table_type == TABLE_TYPE_UNLOGGED) {
        sb_append(sb, "UNLOGGED ");
    }

    sb_append(sb, "TABLE ");

    if (stmt->if_not_exists) {
        sb_append(sb, "IF NOT EXISTS ");
    }

    char *quoted_table = quote_identifier(stmt->table_name);
    sb_append(sb, quoted_table);
    free(quoted_table);

    /* Handle different table variants */
    if (stmt->variant == CREATE_TABLE_REGULAR && stmt->table_def.regular.elements) {
        sb_append(sb, " (\n");

        bool first = true;
        for (TableElement *elem = stmt->table_def.regular.elements; elem; elem = elem->next) {
            /* Skip foreign key table constraints if requested */
            if (skip_foreign_keys && elem->type == TABLE_ELEM_TABLE_CONSTRAINT) {
                TableConstraint *tc = elem->elem.table_constraint;
                if (tc && tc->type == TABLE_CONSTRAINT_FOREIGN_KEY) {
                    continue;
                }
            }

            if (!first) {
                sb_append(sb, ",\n");
            }
            first = false;

            sb_append(sb, "    ");

            if (elem->type == TABLE_ELEM_COLUMN) {
                ColumnDef *col = &elem->elem.column;

                /* Column name and type */
                char *quoted_col = quote_identifier(col->column_name);
                sb_append(sb, quoted_col);
                free(quoted_col);

                sb_append(sb, " ");
                sb_append(sb, col->data_type ? col->data_type : "text");

                /* Column constraints */
                for (ColumnConstraint *cc = col->constraints; cc; cc = cc->next) {
                    /* Skip foreign key column constraints if requested */
                    if (skip_foreign_keys && cc->type == CONSTRAINT_REFERENCES) {
                        continue;
                    }
                    sb_append(sb, " ");
                    char *constraint_sql = generate_column_constraint(cc);
                    if (constraint_sql) {
                        sb_append(sb, constraint_sql);
                        free(constraint_sql);
                    }
                }
            } else if (elem->type == TABLE_ELEM_TABLE_CONSTRAINT) {
                char *constraint_sql = generate_table_constraint(elem->elem.table_constraint);
                if (constraint_sql) {
                    sb_append(sb, constraint_sql);
                    free(constraint_sql);
                }
            }
        }

        sb_append(sb, "\n)");
    } else {
        sb_append(sb, " ()");
    }

    /* INHERITS clause */
    if (stmt->variant == CREATE_TABLE_REGULAR && stmt->table_def.regular.inherits_count > 0) {
        sb_append(sb, " INHERITS (");
        for (int i = 0; i < stmt->table_def.regular.inherits_count; i++) {
            if (i > 0) sb_append(sb, ", ");
            char *quoted = quote_identifier(stmt->table_def.regular.inherits[i]);
            sb_append(sb, quoted);
            free(quoted);
        }
        sb_append(sb, ")");
    }

    /* WITH options */
    if (stmt->with_options && stmt->with_options->count > 0) {
        sb_append(sb, " WITH (");
        for (int i = 0; i < stmt->with_options->count; i++) {
            if (i > 0) sb_append(sb, ", ");
            sb_append(sb, stmt->with_options->parameters[i].name);
            if (stmt->with_options->parameters[i].value) {
                sb_append(sb, "=");
                sb_append(sb, stmt->with_options->parameters[i].value);
            }
        }
        sb_append(sb, ")");
    }

    /* TABLESPACE */
    if (stmt->tablespace_name) {
        sb_append(sb, " TABLESPACE ");
        char *quoted_ts = quote_identifier(stmt->tablespace_name);
        sb_append(sb, quoted_ts);
        free(quoted_ts);
    }

    sb_append(sb, ";\n");

    char *result = sb_to_string(sb);
    sb_free(sb);
    return result;
}

/* Generate CREATE TABLE SQL */
char *generate_create_table_sql(const CreateTableStmt *stmt, const SQLGenOptions *opts) {
    return generate_create_table_sql_internal(stmt, opts, false);
}

/* Extract and generate ALTER TABLE ADD CONSTRAINT statements for foreign keys (used by sql_generator.c) */
char *generate_foreign_key_constraints(const CreateTableStmt *stmt, const SQLGenOptions *opts) {
    if (!stmt || !stmt->table_name || stmt->variant != CREATE_TABLE_REGULAR) {
        return NULL;
    }

    StringBuilder *sb = sb_create();
    if (!sb) {
        return NULL;
    }

    char *quoted_table = quote_identifier(stmt->table_name);
    bool has_fks = false;

    /* Iterate through table elements */
    for (TableElement *elem = stmt->table_def.regular.elements; elem; elem = elem->next) {
        if (elem->type == TABLE_ELEM_COLUMN) {
            /* Check column constraints for foreign keys */
            ColumnDef *col = &elem->elem.column;
            for (ColumnConstraint *cc = col->constraints; cc; cc = cc->next) {
                if (cc->type == CONSTRAINT_REFERENCES) {
                    has_fks = true;

                    if (opts->add_comments) {
                        sb_append(sb, "-- Add foreign key constraint for column ");
                        sb_append(sb, col->column_name);
                        sb_append(sb, "\n");
                    }

                    sb_append(sb, "ALTER TABLE ");
                    sb_append(sb, quoted_table);
                    sb_append(sb, " ADD ");

                    /* Add constraint name if present */
                    if (cc->constraint_name) {
                        sb_append(sb, "CONSTRAINT ");
                        char *quoted_name = quote_identifier(cc->constraint_name);
                        sb_append(sb, quoted_name);
                        free(quoted_name);
                        sb_append(sb, " ");
                    }

                    sb_append(sb, "FOREIGN KEY (");
                    char *quoted_col = quote_identifier(col->column_name);
                    sb_append(sb, quoted_col);
                    free(quoted_col);
                    sb_append(sb, ") ");

                    /* Generate references clause (without CONSTRAINT keyword) */
                    char *ref_sql = generate_references_clause(cc);
                    if (ref_sql) {
                        sb_append(sb, ref_sql);
                        free(ref_sql);
                    }

                    sb_append(sb, ";\n");
                }
            }
        } else if (elem->type == TABLE_ELEM_TABLE_CONSTRAINT) {
            /* Check table constraints for foreign keys */
            TableConstraint *tc = elem->elem.table_constraint;
            if (tc && tc->type == TABLE_CONSTRAINT_FOREIGN_KEY) {
                has_fks = true;

                if (opts->add_comments) {
                    sb_append(sb, "-- Add foreign key table constraint\n");
                }

                sb_append(sb, "ALTER TABLE ");
                sb_append(sb, quoted_table);
                sb_append(sb, " ADD ");

                /* Generate full constraint SQL */
                char *constraint_sql = generate_table_constraint(tc);
                if (constraint_sql) {
                    sb_append(sb, constraint_sql);
                    free(constraint_sql);
                }

                sb_append(sb, ";\n");
            }
        }
    }

    free(quoted_table);

    if (!has_fks) {
        sb_free(sb);
        return NULL;
    }

    char *result = sb_to_string(sb);
    sb_free(sb);
    return result;
}

/* Generate DROP TABLE SQL */
char *generate_drop_table_sql(const char *table_name, const SQLGenOptions *opts) {
    if (!table_name) {
        return NULL;
    }

    StringBuilder *sb = sb_create();
    if (!sb) {
        return NULL;
    }

    if (opts->add_warnings) {
        sb_append(sb, "-- WARNING: Dropping table - all data will be lost\n");
    }

    if (opts->add_comments) {
        sb_append(sb, "-- Drop table ");
        sb_append(sb, table_name);
        sb_append(sb, "\n");
    }

    char *quoted_table = quote_identifier(table_name);

    sb_append(sb, "DROP TABLE ");
    if (opts->use_if_exists) {
        sb_append(sb, "IF EXISTS ");
    }
    sb_append(sb, quoted_table);
    sb_append(sb, " CASCADE;\n");

    free(quoted_table);

    char *result = sb_to_string(sb);
    sb_free(sb);
    return result;
}
