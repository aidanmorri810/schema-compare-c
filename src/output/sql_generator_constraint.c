#include "sql_generator.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Helper: Append constraint name if present */
static void append_constraint_name(StringBuilder *sb, const char *name) {
    if (!sb || !name) {
        return;
    }
    sb_append(sb, "CONSTRAINT ");
    sb_append_identifier(sb, name);
    sb_append(sb, " ");
}

/* Helper: Generate just the REFERENCES clause from a column constraint (without CONSTRAINT name) */
void generate_references_clause(StringBuilder *sb, const ColumnConstraint *cc) {
    if (!sb || !cc || cc->type != CONSTRAINT_REFERENCES) {
        return;
    }

    sb_append(sb, "REFERENCES ");
    if (cc->constraint.references.reftable) {
        sb_append_identifier(sb, cc->constraint.references.reftable);
    }
    if (cc->constraint.references.refcolumn) {
        sb_append(sb, " (");
        sb_append_identifier(sb, cc->constraint.references.refcolumn);
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
}

/* Helper: Generate column constraint SQL (used by sql_generator_table.c) */
void generate_column_constraint(StringBuilder *sb, const ColumnConstraint *cc) {
    if (!sb || !cc) {
        return;
    }

    append_constraint_name(sb, cc->constraint_name);

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
            generate_references_clause(sb, cc);
            break;
        }
        default:
            break;
    }
}

/* Helper: Generate table constraint SQL (used by sql_generator_table.c
 * If include_constraint_name is true, includes "CONSTRAINT <name>" prefix
 * If include_constraint_name is false, generates only the constraint definition
 */
static void generate_table_constraint_internal(StringBuilder *sb, const TableConstraint *tc, bool include_constraint_name) {
    if (!sb || !tc) {
        return;
    }

    if (include_constraint_name) {
        append_constraint_name(sb, tc->constraint_name);
    }

    switch (tc->type) {
        case TABLE_CONSTRAINT_PRIMARY_KEY:
            sb_append(sb, "PRIMARY KEY (");
            for (int i = 0; i < tc->constraint.primary_key.column_count; i++) {
                if (i > 0) sb_append(sb, ", ");
                sb_append_identifier(sb, tc->constraint.primary_key.columns[i]);
            }
            sb_append(sb, ")");
            break;
        case TABLE_CONSTRAINT_UNIQUE:
            sb_append(sb, "UNIQUE (");
            for (int i = 0; i < tc->constraint.unique.column_count; i++) {
                if (i > 0) sb_append(sb, ", ");
                sb_append_identifier(sb, tc->constraint.unique.columns[i]);
            }
            sb_append(sb, ")");
            break;
        case TABLE_CONSTRAINT_FOREIGN_KEY:
            sb_append(sb, "FOREIGN KEY (");
            for (int i = 0; i < tc->constraint.foreign_key.column_count; i++) {
                if (i > 0) sb_append(sb, ", ");
                sb_append_identifier(sb, tc->constraint.foreign_key.columns[i]);
            }
            sb_append(sb, ") REFERENCES ");
            if (tc->constraint.foreign_key.reftable) {
                sb_append_identifier(sb, tc->constraint.foreign_key.reftable);
            }
            if (tc->constraint.foreign_key.refcolumn_count > 0) {
                sb_append(sb, " (");
                for (int i = 0; i < tc->constraint.foreign_key.refcolumn_count; i++) {
                    if (i > 0) sb_append(sb, ", ");
                    sb_append_identifier(sb, tc->constraint.foreign_key.refcolumns[i]);
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
}

/* Public wrapper: Generate table constraint SQL with CONSTRAINT name (used by sql_generator_table.c) */
void generate_table_constraint(StringBuilder *sb, const TableConstraint *tc) {
    generate_table_constraint_internal(sb, tc, true);
}

/* Helper: Generate constraint definition from ConstraintDiff */
static void generate_constraint_definition(StringBuilder *sb, const ConstraintDiff *cd) {
    if (!sb || !cd) {
        return;
    }

    /* Use target_constraint if available for added constraints */
    if (cd->added && cd->target_constraint) {
        if (!cd->is_column_constraint) {
            /* Table-level constraint - generate definition without CONSTRAINT keyword */
            const TableConstraint *tc = (const TableConstraint *)cd->target_constraint;
            generate_table_constraint_internal(sb, tc, false);
        } else {
            /* Column-level constraint - generate as table-level */
            const ColumnConstraint *cc = (const ColumnConstraint *)cd->target_constraint;

            /* For column-level UNIQUE/PK, convert to table-level syntax */
            if (cc->type == CONSTRAINT_UNIQUE) {
                sb_append(sb, "UNIQUE (");
                if (cd->column_name) {
                    sb_append_identifier(sb, cd->column_name);
                }
                sb_append(sb, ")");
            } else if (cc->type == CONSTRAINT_PRIMARY_KEY) {
                sb_append(sb, "PRIMARY KEY (");
                if (cd->column_name) {
                    sb_append_identifier(sb, cd->column_name);
                }
                sb_append(sb, ")");
            } else if (cc->type == CONSTRAINT_REFERENCES) {
                /* For REFERENCES, generate FOREIGN KEY with column and references clause */
                sb_append(sb, "FOREIGN KEY (");
                if (cd->column_name) {
                    sb_append_identifier(sb, cd->column_name);
                }
                sb_append(sb, ") ");
                /* Use generate_references_clause to avoid duplicate CONSTRAINT keyword */
                generate_references_clause(sb, cc);
            } else {
                /* For other column constraints, generate inline syntax */
                generate_column_constraint(sb, cc);
            }
        }
        return;
    }

    /* Use new_definition if available */
    if (cd->new_definition) {
        sb_append(sb, cd->new_definition);
        return;
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

    if (type_str) {
        sb_append(sb, type_str);
    }
}

/* Generate DROP CONSTRAINT SQL */
void generate_drop_constraint_sql(StringBuilder *sb, const char *table_name, const char *constraint_name,
                                   const SQLGenOptions *opts) {
    if (!sb || !table_name || !constraint_name) {
        return;
    }

    if (opts->add_warnings) {
        sb_append(sb, "-- WARNING: Dropping constraint\n");
    }

    if (opts->add_comments) {
        sb_append(sb, "-- Drop constraint ");
        sb_append(sb, constraint_name);
        sb_append(sb, "\n");
    }

    sb_append(sb, "ALTER TABLE ");
    sb_append_identifier(sb, table_name);
    sb_append(sb, " DROP CONSTRAINT ");
    if (opts->use_if_exists) {
        sb_append(sb, "IF EXISTS ");
    }
    sb_append_identifier(sb, constraint_name);
    sb_append(sb, ";\n");
}

/* Generate ADD CONSTRAINT SQL */
void generate_add_constraint_sql(StringBuilder *sb, const char *table_name, const ConstraintDiff *constraint,
                                  const SQLGenOptions *opts) {
    if (!sb || !table_name || !constraint) {
        return;
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

    sb_append(sb, "ALTER TABLE ");
    sb_append_identifier(sb, table_name);
    sb_append(sb, " ADD ");

    /* Add constraint name if present */
    if (constraint->constraint_name) {
        sb_append(sb, "CONSTRAINT ");
        sb_append_identifier(sb, constraint->constraint_name);
        sb_append(sb, " ");
    }

    /* Add constraint definition */
    generate_constraint_definition(sb, constraint);

    sb_append(sb, ";\n");
}
