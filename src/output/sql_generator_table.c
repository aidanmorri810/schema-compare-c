#include "sql_generator.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

/* Forward declarations for constraint helpers (defined in sql_generator_constraint.c) */
void generate_column_constraint(StringBuilder *sb, const ColumnConstraint *cc);
void generate_table_constraint(StringBuilder *sb, const TableConstraint *tc);
void generate_references_clause(StringBuilder *sb, const ColumnConstraint *cc);

/* Internal function to generate CREATE TABLE SQL with option to skip foreign keys (used by sql_generator.c) */
void generate_create_table_sql_internal(StringBuilder *sb, const CreateTableStmt *stmt, const SQLGenOptions *opts, bool skip_foreign_keys) {
    if (!sb || !stmt || !stmt->table_name) {
        return;
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

    sb_append_identifier(sb, stmt->table_name);

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
                sb_append_identifier(sb, col->column_name);

                sb_append(sb, " ");
                sb_append(sb, col->data_type ? col->data_type : "text");

                /* Column constraints */
                for (ColumnConstraint *cc = col->constraints; cc; cc = cc->next) {
                    /* Skip foreign key column constraints if requested */
                    if (skip_foreign_keys && cc->type == CONSTRAINT_REFERENCES) {
                        continue;
                    }
                    sb_append(sb, " ");
                    generate_column_constraint(sb, cc);
                }
            } else if (elem->type == TABLE_ELEM_TABLE_CONSTRAINT) {
                generate_table_constraint(sb, elem->elem.table_constraint);
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
            sb_append_identifier(sb, stmt->table_def.regular.inherits[i]);
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
        sb_append_identifier(sb, stmt->tablespace_name);
    }

    sb_append(sb, ";\n");
}

/* Generate CREATE TABLE SQL */
void generate_create_table_sql(StringBuilder *sb, const CreateTableStmt *stmt, const SQLGenOptions *opts) {
    generate_create_table_sql_internal(sb, stmt, opts, false);
}

/* Extract and generate ALTER TABLE ADD CONSTRAINT statements for foreign keys (used by sql_generator.c) */
void generate_foreign_key_constraints(StringBuilder *sb, const CreateTableStmt *stmt, const SQLGenOptions *opts) {
    if (!sb || !stmt || !stmt->table_name || stmt->variant != CREATE_TABLE_REGULAR) {
        return;
    }

    /* Iterate through table elements */
    for (TableElement *elem = stmt->table_def.regular.elements; elem; elem = elem->next) {
        if (elem->type == TABLE_ELEM_COLUMN) {
            /* Check column constraints for foreign keys */
            ColumnDef *col = &elem->elem.column;
            for (ColumnConstraint *cc = col->constraints; cc; cc = cc->next) {
                if (cc->type == CONSTRAINT_REFERENCES) {
                    if (opts->add_comments) {
                        sb_append(sb, "-- Add foreign key constraint for column ");
                        sb_append(sb, col->column_name);
                        sb_append(sb, "\n");
                    }

                    sb_append(sb, "ALTER TABLE ");
                    sb_append_identifier(sb, stmt->table_name);
                    sb_append(sb, " ADD ");

                    /* Add constraint name if present */
                    if (cc->constraint_name) {
                        sb_append(sb, "CONSTRAINT ");
                        sb_append_identifier(sb, cc->constraint_name);
                        sb_append(sb, " ");
                    }

                    sb_append(sb, "FOREIGN KEY (");
                    sb_append_identifier(sb, col->column_name);
                    sb_append(sb, ") ");

                    /* Generate references clause (without CONSTRAINT keyword) */
                    generate_references_clause(sb, cc);

                    sb_append(sb, ";\n");
                }
            }
        } else if (elem->type == TABLE_ELEM_TABLE_CONSTRAINT) {
            /* Check table constraints for foreign keys */
            TableConstraint *tc = elem->elem.table_constraint;
            if (tc && tc->type == TABLE_CONSTRAINT_FOREIGN_KEY) {
                if (opts->add_comments) {
                    sb_append(sb, "-- Add foreign key table constraint\n");
                }

                sb_append(sb, "ALTER TABLE ");
                sb_append_identifier(sb, stmt->table_name);
                sb_append(sb, " ADD ");

                /* Generate full constraint SQL */
                generate_table_constraint(sb, tc);

                sb_append(sb, ";\n");
            }
        }
    }
}

/* Generate DROP TABLE SQL */
void generate_drop_table_sql(StringBuilder *sb, const char *table_name, const SQLGenOptions *opts) {
    if (!sb || !table_name) {
        return;
    }

    if (opts->add_warnings) {
        sb_append(sb, "-- WARNING: Dropping table - all data will be lost\n");
    }

    if (opts->add_comments) {
        sb_append(sb, "-- Drop table ");
        sb_append(sb, table_name);
        sb_append(sb, "\n");
    }

    sb_append(sb, "DROP TABLE ");
    if (opts->use_if_exists) {
        sb_append(sb, "IF EXISTS ");
    }
    sb_append_identifier(sb, table_name);
    sb_append(sb, " CASCADE;\n");
}
