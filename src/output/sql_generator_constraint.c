#include "sql_generator.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

/* Helper: Generate just the REFERENCES clause from a column constraint (without CONSTRAINT name) */
char *generate_references_clause(const ColumnConstraint *cc) {
    if (!cc || cc->type != CONSTRAINT_REFERENCES) {
        return NULL;
    }

    StringBuilder *sb = sb_create();
    if (!sb) {
        return NULL;
    }

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

    char *result = sb_to_string(sb);
    sb_free(sb);
    return result;
}

/* Helper: Generate column constraint SQL (used by sql_generator_table.c) */
char *generate_column_constraint(const ColumnConstraint *cc) {
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
        case CONSTRAINT_REFERENCES: {
            char *ref_clause = generate_references_clause(cc);
            if (ref_clause) {
                sb_append(sb, ref_clause);
                free(ref_clause);
            }
            break;
        }
        default:
            break;
    }

    char *result = sb_to_string(sb);
    sb_free(sb);
    return result;
}

/* Helper: Generate table constraint SQL (used by sql_generator_table.c
 * If include_constraint_name is true, includes "CONSTRAINT <name>" prefix
 * If include_constraint_name is false, generates only the constraint definition
 */
static char *generate_table_constraint_internal(const TableConstraint *tc, bool include_constraint_name) {
    if (!tc) {
        return NULL;
    }

    StringBuilder *sb = sb_create();
    if (!sb) {
        return NULL;
    }

    if (include_constraint_name) {
        char *constraint_name = format_constraint_name(tc->constraint_name);
        if (constraint_name) {
            sb_append(sb, constraint_name);
            free(constraint_name);
        }
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

/* Public wrapper: Generate table constraint SQL with CONSTRAINT name (used by sql_generator_table.c) */
char *generate_table_constraint(const TableConstraint *tc) {
    return generate_table_constraint_internal(tc, true);
}

/* Helper: Generate constraint definition from ConstraintDiff */
static char *generate_constraint_definition(const ConstraintDiff *cd) {
    if (!cd) {
        return NULL;
    }

    /* Use target_constraint if available for added constraints */
    if (cd->added && cd->target_constraint) {
        if (!cd->is_column_constraint) {
            /* Table-level constraint - generate definition without CONSTRAINT keyword */
            const TableConstraint *tc = (const TableConstraint *)cd->target_constraint;
            return generate_table_constraint_internal(tc, false);
        } else {
            /* Column-level constraint - generate as table-level */
            const ColumnConstraint *cc = (const ColumnConstraint *)cd->target_constraint;

            StringBuilder *sb = sb_create();
            if (!sb) return NULL;

            /* For column-level UNIQUE/PK, convert to table-level syntax */
            if (cc->type == CONSTRAINT_UNIQUE) {
                sb_append(sb, "UNIQUE (");
                if (cd->column_name) {
                    char *quoted = quote_identifier(cd->column_name);
                    sb_append(sb, quoted);
                    free(quoted);
                }
                sb_append(sb, ")");
            } else if (cc->type == CONSTRAINT_PRIMARY_KEY) {
                sb_append(sb, "PRIMARY KEY (");
                if (cd->column_name) {
                    char *quoted = quote_identifier(cd->column_name);
                    sb_append(sb, quoted);
                    free(quoted);
                }
                sb_append(sb, ")");
            } else if (cc->type == CONSTRAINT_REFERENCES) {
                /* For REFERENCES, generate FOREIGN KEY with column and references clause */
                sb_append(sb, "FOREIGN KEY (");
                if (cd->column_name) {
                    char *quoted = quote_identifier(cd->column_name);
                    sb_append(sb, quoted);
                    free(quoted);
                }
                sb_append(sb, ") ");
                /* Use generate_references_clause to avoid duplicate CONSTRAINT keyword */
                char *ref_sql = generate_references_clause(cc);
                if (ref_sql) {
                    sb_append(sb, ref_sql);
                    free(ref_sql);
                }
            } else {
                /* For other column constraints, generate inline syntax */
                char *constraint_sql = generate_column_constraint(cc);
                if (constraint_sql) {
                    sb_append(sb, constraint_sql);
                    free(constraint_sql);
                }
            }

            char *result = sb_to_string(sb);
            sb_free(sb);
            return result;
        }
    }

    /* Use new_definition if available */
    if (cd->new_definition) {
        return strdup(cd->new_definition);
    }

    /* Fallback: Generate based on type */
    const char *type_str = NULL;
    int constraint_type = cd->added ? cd->new_type : cd->old_type;

    switch (constraint_type) {
        case TABLE_CONSTRAINT_CHECK:
            type_str = "CHECK (...)";
            break;
        case TABLE_CONSTRAINT_UNIQUE:
            type_str = "UNIQUE (...)";
            break;
        case TABLE_CONSTRAINT_PRIMARY_KEY:
            type_str = "PRIMARY KEY (...)";
            break;
        case TABLE_CONSTRAINT_FOREIGN_KEY:
            type_str = "FOREIGN KEY (...) REFERENCES ...";
            break;
        case TABLE_CONSTRAINT_EXCLUDE:
            type_str = "EXCLUDE ...";
            break;
        default:
            type_str = "CONSTRAINT";
            break;
    }

    return type_str ? strdup(type_str) : NULL;
}

/* Generate DROP CONSTRAINT SQL */
char *generate_drop_constraint_sql(const char *table_name, const char *constraint_name,
                                    const SQLGenOptions *opts) {
    if (!table_name || !constraint_name) {
        return NULL;
    }

    StringBuilder *sb = sb_create();
    if (!sb) {
        return NULL;
    }

    if (opts->add_warnings) {
        sb_append(sb, "-- WARNING: Dropping constraint\n");
    }

    if (opts->add_comments) {
        sb_append(sb, "-- Drop constraint ");
        sb_append(sb, constraint_name);
        sb_append(sb, "\n");
    }

    char *quoted_table = quote_identifier(table_name);
    char *quoted_constraint = quote_identifier(constraint_name);

    sb_append(sb, "ALTER TABLE ");
    sb_append(sb, quoted_table);
    sb_append(sb, " DROP CONSTRAINT ");
    if (opts->use_if_exists) {
        sb_append(sb, "IF EXISTS ");
    }
    sb_append(sb, quoted_constraint);
    sb_append(sb, ";\n");

    free(quoted_table);
    free(quoted_constraint);

    char *result = sb_to_string(sb);
    sb_free(sb);
    return result;
}

/* Generate ADD CONSTRAINT SQL */
char *generate_add_constraint_sql(const char *table_name, const ConstraintDiff *constraint,
                                   const SQLGenOptions *opts) {
    if (!table_name || !constraint) {
        return NULL;
    }

    StringBuilder *sb = sb_create();
    if (!sb) {
        return NULL;
    }

    if (opts->add_comments) {
        sb_append(sb, "-- Add constraint ");
        if (constraint->constraint_name) {
            sb_append(sb, constraint->constraint_name);
        } else {
            sb_append(sb, "(unnamed)");
        }
        sb_append(sb, "\n");
    }

    char *quoted_table = quote_identifier(table_name);

    sb_append(sb, "ALTER TABLE ");
    sb_append(sb, quoted_table);
    sb_append(sb, " ADD ");

    /* Add constraint name if present */
    if (constraint->constraint_name) {
        sb_append(sb, "CONSTRAINT ");
        char *quoted_name = quote_identifier(constraint->constraint_name);
        sb_append(sb, quoted_name);
        free(quoted_name);
        sb_append(sb, " ");
    }

    /* Add constraint definition */
    char *definition = generate_constraint_definition(constraint);
    if (definition) {
        sb_append(sb, definition);
        free(definition);
    }

    sb_append(sb, ";\n");

    free(quoted_table);

    char *result = sb_to_string(sb);
    sb_free(sb);
    return result;
}
