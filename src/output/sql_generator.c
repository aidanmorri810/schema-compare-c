#include "sql_generator.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* Initialize default SQL generation options */
SQLGenOptions *sql_gen_options_default(void) {
    SQLGenOptions *opts = calloc(1, sizeof(SQLGenOptions));
    if (!opts) {
        return NULL;
    }

    opts->use_transactions = true;
    opts->use_if_exists = true;
    opts->add_comments = true;
    opts->add_warnings = true;
    opts->generate_rollback = false;
    opts->safe_mode = true;
    opts->schema_name = NULL;

    return opts;
}

void sql_gen_options_free(SQLGenOptions *opts) {
    free(opts);
}

/* Free SQL migration */
void sql_migration_free(SQLMigration *migration) {
    if (!migration) {
        return;
    }

    free(migration->forward_sql);
    free(migration->rollback_sql);
    free(migration);
}

/* Quote SQL identifier */
char *quote_identifier(const char *identifier) {
    if (!identifier) {
        return NULL;
    }

    /* Check if identifier needs quoting */
    bool needs_quote = false;

    /* Check if it contains special characters (except underscore) */
    for (const char *p = identifier; *p; p++) {
        if (!isalnum(*p) && *p != '_') {
            needs_quote = true;
            break;
        }
    }

    /* Check if it starts with a digit */
    if (!needs_quote && isdigit(identifier[0])) {
        needs_quote = true;
    }

    /* Don't quote if not necessary */
    if (!needs_quote) {
        return strdup(identifier);
    }

    /* Quote and escape */
    size_t len = strlen(identifier);
    char *quoted = malloc(len * 2 + 3);  /* Worst case: all quotes + surrounding quotes */
    if (!quoted) {
        return NULL;
    }

    char *dst = quoted;
    *dst++ = '"';

    for (const char *src = identifier; *src; src++) {
        if (*src == '"') {
            *dst++ = '"';
            *dst++ = '"';
        } else {
            *dst++ = *src;
        }
    }

    *dst++ = '"';
    *dst = '\0';

    return quoted;
}

/* Quote SQL literal */
char *quote_literal(const char *literal) {
    if (!literal) {
        return strdup("NULL");
    }

    size_t len = strlen(literal);
    char *quoted = malloc(len * 2 + 3);
    if (!quoted) {
        return NULL;
    }

    char *dst = quoted;
    *dst++ = '\'';

    for (const char *src = literal; *src; src++) {
        if (*src == '\'') {
            *dst++ = '\'';
            *dst++ = '\'';
        } else {
            *dst++ = *src;
        }
    }

    *dst++ = '\'';
    *dst = '\0';

    return quoted;
}

/* Format data type */
char *format_data_type(const char *type) {
    if (!type) {
        return NULL;
    }
    return strdup(type);
}

/* Generate ADD COLUMN SQL */
char *generate_add_column_sql(const char *table_name, const ColumnDiff *col,
                              const SQLGenOptions *opts) {
    if (!table_name || !col) {
        return NULL;
    }

    StringBuilder *sb = sb_create();
    if (!sb) {
        return NULL;
    }

    if (opts->add_comments) {
        sb_append(sb, "-- Add column ");
        sb_append(sb, col->column_name);
        sb_append(sb, "\n");
    }

    char *quoted_table = quote_identifier(table_name);
    char *quoted_column = quote_identifier(col->column_name);

    sb_append(sb, "ALTER TABLE ");
    sb_append(sb, quoted_table);
    sb_append(sb, " ADD COLUMN ");
    sb_append(sb, quoted_column);
    sb_append(sb, " ");
    sb_append(sb, col->new_type ? col->new_type : "text");
    sb_append(sb, ";\n");

    free(quoted_table);
    free(quoted_column);

    char *result = sb_to_string(sb);
    sb_free(sb);
    return result;
}

/* Generate DROP COLUMN SQL */
char *generate_drop_column_sql(const char *table_name, const char *column_name,
                               const SQLGenOptions *opts) {
    if (!table_name || !column_name) {
        return NULL;
    }

    StringBuilder *sb = sb_create();
    if (!sb) {
        return NULL;
    }

    if (opts->add_warnings) {
        sb_append(sb, "-- WARNING: Dropping column - potential data loss\n");
    }

    if (opts->add_comments) {
        sb_append(sb, "-- Drop column ");
        sb_append(sb, column_name);
        sb_append(sb, "\n");
    }

    char *quoted_table = quote_identifier(table_name);
    char *quoted_column = quote_identifier(column_name);

    sb_append(sb, "ALTER TABLE ");
    sb_append(sb, quoted_table);
    sb_append(sb, " DROP COLUMN ");
    if (opts->use_if_exists) {
        sb_append(sb, "IF EXISTS ");
    }
    sb_append(sb, quoted_column);
    sb_append(sb, ";\n");

    free(quoted_table);
    free(quoted_column);

    char *result = sb_to_string(sb);
    sb_free(sb);
    return result;
}

/* Generate ALTER COLUMN TYPE SQL */
char *generate_alter_column_type_sql(const char *table_name, const ColumnDiff *col,
                                     const SQLGenOptions *opts) {
    if (!table_name || !col) {
        return NULL;
    }

    StringBuilder *sb = sb_create();
    if (!sb) {
        return NULL;
    }

    if (opts->add_warnings) {
        sb_append(sb, "-- WARNING: Changing column type may cause data conversion issues\n");
    }

    if (opts->add_comments) {
        sb_append_fmt(sb, "-- Change column type: %s → %s\n",
                     col->old_type ? col->old_type : "unknown",
                     col->new_type ? col->new_type : "unknown");
    }

    char *quoted_table = quote_identifier(table_name);
    char *quoted_column = quote_identifier(col->column_name);

    sb_append(sb, "ALTER TABLE ");
    sb_append(sb, quoted_table);
    sb_append(sb, " ALTER COLUMN ");
    sb_append(sb, quoted_column);
    sb_append(sb, " TYPE ");
    sb_append(sb, col->new_type ? col->new_type : "text");
    sb_append(sb, ";\n");

    free(quoted_table);
    free(quoted_column);

    char *result = sb_to_string(sb);
    sb_free(sb);
    return result;
}

/* Generate ALTER COLUMN SET/DROP NOT NULL SQL */
char *generate_alter_column_nullable_sql(const char *table_name, const ColumnDiff *col,
                                         const SQLGenOptions *opts) {
    if (!table_name || !col) {
        return NULL;
    }

    StringBuilder *sb = sb_create();
    if (!sb) {
        return NULL;
    }

    if (opts->add_comments) {
        sb_append_fmt(sb, "-- Change nullability: %s → %s\n",
                     col->old_nullable ? "NULL" : "NOT NULL",
                     col->new_nullable ? "NULL" : "NOT NULL");
    }

    char *quoted_table = quote_identifier(table_name);
    char *quoted_column = quote_identifier(col->column_name);

    sb_append(sb, "ALTER TABLE ");
    sb_append(sb, quoted_table);
    sb_append(sb, " ALTER COLUMN ");
    sb_append(sb, quoted_column);

    if (col->new_nullable) {
        sb_append(sb, " DROP NOT NULL");
    } else {
        sb_append(sb, " SET NOT NULL");
    }

    sb_append(sb, ";\n");

    free(quoted_table);
    free(quoted_column);

    char *result = sb_to_string(sb);
    sb_free(sb);
    return result;
}

/* Generate ALTER COLUMN SET/DROP DEFAULT SQL */
char *generate_alter_column_default_sql(const char *table_name, const ColumnDiff *col,
                                        const SQLGenOptions *opts) {
    if (!table_name || !col) {
        return NULL;
    }

    StringBuilder *sb = sb_create();
    if (!sb) {
        return NULL;
    }

    if (opts->add_comments) {
        sb_append_fmt(sb, "-- Change default: %s → %s\n",
                     col->old_default ? col->old_default : "(none)",
                     col->new_default ? col->new_default : "(none)");
    }

    char *quoted_table = quote_identifier(table_name);
    char *quoted_column = quote_identifier(col->column_name);

    sb_append(sb, "ALTER TABLE ");
    sb_append(sb, quoted_table);
    sb_append(sb, " ALTER COLUMN ");
    sb_append(sb, quoted_column);

    if (col->new_default) {
        sb_append(sb, " SET DEFAULT ");
        sb_append(sb, col->new_default);
    } else {
        sb_append(sb, " DROP DEFAULT");
    }

    sb_append(sb, ";\n");

    free(quoted_table);
    free(quoted_column);

    char *result = sb_to_string(sb);
    sb_free(sb);
    return result;
}

/* Helper: Generate constraint name or empty string */
static char *format_constraint_name(const char *name) {
    if (!name) {
        return strdup("");
    }
    char *quoted = quote_identifier(name);
    if (!quoted) {
        return strdup("");
    }
    char *result = malloc(strlen(quoted) + 20);
    if (!result) {
        free(quoted);
        return strdup("");
    }
    sprintf(result, "CONSTRAINT %s ", quoted);
    free(quoted);
    return result;
}

/* Helper: Generate column constraint SQL */
static char *generate_column_constraint(const ColumnConstraint *cc) {
    if (!cc) {
        return NULL;
    }

    StringBuilder *sb = sb_create();
    if (!sb) {
        return NULL;
    }

    char *constraint_name = format_constraint_name(cc->constraint_name);
    if (constraint_name) {
        sb_append(sb, constraint_name);
        free(constraint_name);
    }

    switch (cc->type) {
        case CONSTRAINT_NOT_NULL:
            sb_append(sb, "NOT NULL");
            break;
        case CONSTRAINT_NULL:
            sb_append(sb, "NULL");
            break;
        case CONSTRAINT_DEFAULT:
            sb_append(sb, "DEFAULT ");
            if (cc->constraint.default_val.expr && cc->constraint.default_val.expr->expression) {
                sb_append(sb, cc->constraint.default_val.expr->expression);
            }
            break;
        case CONSTRAINT_CHECK:
            sb_append(sb, "CHECK (");
            if (cc->constraint.check.expr && cc->constraint.check.expr->expression) {
                sb_append(sb, cc->constraint.check.expr->expression);
            }
            sb_append(sb, ")");
            break;
        case CONSTRAINT_UNIQUE:
            sb_append(sb, "UNIQUE");
            break;
        case CONSTRAINT_PRIMARY_KEY:
            sb_append(sb, "PRIMARY KEY");
            break;
        case CONSTRAINT_REFERENCES:
            sb_append(sb, "REFERENCES ");
            if (cc->constraint.references.reftable) {
                char *quoted_table = quote_identifier(cc->constraint.references.reftable);
                sb_append(sb, quoted_table);
                free(quoted_table);
            }
            if (cc->constraint.references.refcolumn) {
                sb_append(sb, " (");
                char *quoted_col = quote_identifier(cc->constraint.references.refcolumn);
                sb_append(sb, quoted_col);
                free(quoted_col);
                sb_append(sb, ")");
            }
            if (cc->constraint.references.has_on_delete) {
                sb_append(sb, " ON DELETE ");
                switch (cc->constraint.references.on_delete) {
                    case REF_ACTION_CASCADE: sb_append(sb, "CASCADE"); break;
                    case REF_ACTION_RESTRICT: sb_append(sb, "RESTRICT"); break;
                    case REF_ACTION_SET_NULL: sb_append(sb, "SET NULL"); break;
                    case REF_ACTION_SET_DEFAULT: sb_append(sb, "SET DEFAULT"); break;
                    default: sb_append(sb, "NO ACTION"); break;
                }
            }
            if (cc->constraint.references.has_on_update) {
                sb_append(sb, " ON UPDATE ");
                switch (cc->constraint.references.on_update) {
                    case REF_ACTION_CASCADE: sb_append(sb, "CASCADE"); break;
                    case REF_ACTION_RESTRICT: sb_append(sb, "RESTRICT"); break;
                    case REF_ACTION_SET_NULL: sb_append(sb, "SET NULL"); break;
                    case REF_ACTION_SET_DEFAULT: sb_append(sb, "SET DEFAULT"); break;
                    default: sb_append(sb, "NO ACTION"); break;
                }
            }
            break;
        default:
            break;
    }

    char *result = sb_to_string(sb);
    sb_free(sb);
    return result;
}

/* Helper: Generate table constraint SQL */
static char *generate_table_constraint(const TableConstraint *tc) {
    if (!tc) {
        return NULL;
    }

    StringBuilder *sb = sb_create();
    if (!sb) {
        return NULL;
    }

    char *constraint_name = format_constraint_name(tc->constraint_name);
    if (constraint_name) {
        sb_append(sb, constraint_name);
        free(constraint_name);
    }

    switch (tc->type) {
        case TABLE_CONSTRAINT_PRIMARY_KEY:
            sb_append(sb, "PRIMARY KEY (");
            for (int i = 0; i < tc->constraint.primary_key.column_count; i++) {
                if (i > 0) sb_append(sb, ", ");
                char *quoted = quote_identifier(tc->constraint.primary_key.columns[i]);
                sb_append(sb, quoted);
                free(quoted);
            }
            sb_append(sb, ")");
            break;
        case TABLE_CONSTRAINT_UNIQUE:
            sb_append(sb, "UNIQUE (");
            for (int i = 0; i < tc->constraint.unique.column_count; i++) {
                if (i > 0) sb_append(sb, ", ");
                char *quoted = quote_identifier(tc->constraint.unique.columns[i]);
                sb_append(sb, quoted);
                free(quoted);
            }
            sb_append(sb, ")");
            break;
        case TABLE_CONSTRAINT_FOREIGN_KEY:
            sb_append(sb, "FOREIGN KEY (");
            for (int i = 0; i < tc->constraint.foreign_key.column_count; i++) {
                if (i > 0) sb_append(sb, ", ");
                char *quoted = quote_identifier(tc->constraint.foreign_key.columns[i]);
                sb_append(sb, quoted);
                free(quoted);
            }
            sb_append(sb, ") REFERENCES ");
            if (tc->constraint.foreign_key.reftable) {
                char *quoted_table = quote_identifier(tc->constraint.foreign_key.reftable);
                sb_append(sb, quoted_table);
                free(quoted_table);
            }
            if (tc->constraint.foreign_key.refcolumn_count > 0) {
                sb_append(sb, " (");
                for (int i = 0; i < tc->constraint.foreign_key.refcolumn_count; i++) {
                    if (i > 0) sb_append(sb, ", ");
                    char *quoted = quote_identifier(tc->constraint.foreign_key.refcolumns[i]);
                    sb_append(sb, quoted);
                    free(quoted);
                }
                sb_append(sb, ")");
            }
            if (tc->constraint.foreign_key.has_on_delete) {
                sb_append(sb, " ON DELETE ");
                switch (tc->constraint.foreign_key.on_delete) {
                    case REF_ACTION_CASCADE: sb_append(sb, "CASCADE"); break;
                    case REF_ACTION_RESTRICT: sb_append(sb, "RESTRICT"); break;
                    case REF_ACTION_SET_NULL: sb_append(sb, "SET NULL"); break;
                    case REF_ACTION_SET_DEFAULT: sb_append(sb, "SET DEFAULT"); break;
                    default: sb_append(sb, "NO ACTION"); break;
                }
            }
            if (tc->constraint.foreign_key.has_on_update) {
                sb_append(sb, " ON UPDATE ");
                switch (tc->constraint.foreign_key.on_update) {
                    case REF_ACTION_CASCADE: sb_append(sb, "CASCADE"); break;
                    case REF_ACTION_RESTRICT: sb_append(sb, "RESTRICT"); break;
                    case REF_ACTION_SET_NULL: sb_append(sb, "SET NULL"); break;
                    case REF_ACTION_SET_DEFAULT: sb_append(sb, "SET DEFAULT"); break;
                    default: sb_append(sb, "NO ACTION"); break;
                }
            }
            break;
        case TABLE_CONSTRAINT_CHECK:
            sb_append(sb, "CHECK (");
            if (tc->constraint.check.expr && tc->constraint.check.expr->expression) {
                sb_append(sb, tc->constraint.check.expr->expression);
            }
            sb_append(sb, ")");
            break;
        default:
            break;
    }

    char *result = sb_to_string(sb);
    sb_free(sb);
    return result;
}

/* Generate CREATE TABLE SQL */
char *generate_create_table_sql(const CreateTableStmt *stmt, const SQLGenOptions *opts) {
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

/* Generate migration SQL from diff */
SQLMigration *generate_migration_sql(const SchemaDiff *diff, const SQLGenOptions *opts) {
    if (!diff || !opts) {
        return NULL;
    }

    SQLMigration *migration = calloc(1, sizeof(SQLMigration));
    if (!migration) {
        return NULL;
    }

    StringBuilder *sb = sb_create();
    if (!sb) {
        free(migration);
        return NULL;
    }

    int stmt_count = 0;

    /* Header */
    if (opts->add_comments) {
        sb_append(sb, "-- Schema Migration Script\n");
        sb_append(sb, "-- Generated by schema-compare\n");
        sb_append(sb, "--\n");
        sb_append_fmt(sb, "-- Tables added: %d, removed: %d, modified: %d\n",
                     diff->tables_added, diff->tables_removed, diff->tables_modified);
        sb_append(sb, "\n");
    }

    /* Begin transaction */
    if (opts->use_transactions) {
        sb_append(sb, "BEGIN;\n\n");
    }

    /* Process each table diff */
    for (TableDiff *td = diff->table_diffs; td; td = td->next) {
        /* Handle removed tables */
        if (td->table_removed) {
            char *drop_sql = generate_drop_table_sql(td->table_name, opts);
            if (drop_sql) {
                sb_append(sb, drop_sql);
                sb_append(sb, "\n");
                free(drop_sql);
                stmt_count++;
                migration->has_destructive_changes = true;
            }
            continue;
        }

        /* Handle added tables */
        if (td->table_added) {
            if (td->target_table) {
                char *create_sql = generate_create_table_sql(td->target_table, opts);
                if (create_sql) {
                    sb_append(sb, create_sql);
                    sb_append(sb, "\n");
                    free(create_sql);
                    stmt_count++;
                }
            } else {
                /* Fallback if table definition not available */
                if (opts->add_comments) {
                    sb_append_fmt(sb, "-- TODO: CREATE TABLE %s\n", td->table_name);
                    sb_append(sb, "-- (Table definition not available in diff)\n\n");
                }
            }
            continue;
        }

        /* Handle column changes */
        for (ColumnDiff *cd = td->columns_removed; cd; cd = cd->next) {
            char *drop_sql = generate_drop_column_sql(td->table_name, cd->column_name, opts);
            if (drop_sql) {
                sb_append(sb, drop_sql);
                sb_append(sb, "\n");
                free(drop_sql);
                stmt_count++;
                migration->has_destructive_changes = true;
            }
        }

        for (ColumnDiff *cd = td->columns_added; cd; cd = cd->next) {
            char *add_sql = generate_add_column_sql(td->table_name, cd, opts);
            if (add_sql) {
                sb_append(sb, add_sql);
                sb_append(sb, "\n");
                free(add_sql);
                stmt_count++;
            }
        }

        for (ColumnDiff *cd = td->columns_modified; cd; cd = cd->next) {
            if (cd->type_changed) {
                char *alter_sql = generate_alter_column_type_sql(td->table_name, cd, opts);
                if (alter_sql) {
                    sb_append(sb, alter_sql);
                    sb_append(sb, "\n");
                    free(alter_sql);
                    stmt_count++;
                }
            }

            if (cd->nullable_changed) {
                char *nullable_sql = generate_alter_column_nullable_sql(td->table_name, cd, opts);
                if (nullable_sql) {
                    sb_append(sb, nullable_sql);
                    sb_append(sb, "\n");
                    free(nullable_sql);
                    stmt_count++;
                }
            }

            if (cd->default_changed) {
                char *default_sql = generate_alter_column_default_sql(td->table_name, cd, opts);
                if (default_sql) {
                    sb_append(sb, default_sql);
                    sb_append(sb, "\n");
                    free(default_sql);
                    stmt_count++;
                }
            }
        }

        /* TODO: Handle constraint changes */
    }

    /* Commit transaction */
    if (opts->use_transactions) {
        sb_append(sb, "COMMIT;\n");
    }

    migration->forward_sql = sb_to_string(sb);
    migration->statement_count = stmt_count;
    sb_free(sb);

    return migration;
}

/* Write migration to file */
bool write_migration_to_file(const SQLMigration *migration, const char *filename) {
    if (!migration || !filename || !migration->forward_sql) {
        return false;
    }

    return write_string_to_file(filename, migration->forward_sql);
}
