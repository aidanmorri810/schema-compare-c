#include "compare.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

/* Helper to extract TableConstraint from TableElement */
static const TableConstraint *get_table_constraint(const TableElement *elem) {
    if (elem && elem->type == TABLE_ELEM_TABLE_CONSTRAINT) {
        return elem->elem.table_constraint;
    }
    return NULL;
}

/* Compare two column constraints for equivalence */
bool column_constraints_equivalent(const ColumnConstraint *c1, const ColumnConstraint *c2,
                                  const CompareOptions *opts) {
    if (c1 == c2) {
        return true;
    }
    if (!c1 || !c2) {
        return false;
    }

    /* Must be same type */
    if (c1->type != c2->type) {
        return false;
    }

    switch (c1->type) {
        case CONSTRAINT_NOT_NULL:
        case CONSTRAINT_NULL:
            /* Simple constraints - just type match is enough */
            return true;

        case CONSTRAINT_CHECK:
            /* Compare CHECK expressions */
            if (!c1->constraint.check.expr || !c2->constraint.check.expr) {
                return c1->constraint.check.expr == c2->constraint.check.expr;
            }
            return expressions_equal(c1->constraint.check.expr->expression,
                                   c2->constraint.check.expr->expression, opts);

        case CONSTRAINT_DEFAULT:
            /* Compare DEFAULT expressions */
            if (!c1->constraint.default_val.expr || !c2->constraint.default_val.expr) {
                return c1->constraint.default_val.expr == c2->constraint.default_val.expr;
            }
            return expressions_equal(c1->constraint.default_val.expr->expression,
                                   c2->constraint.default_val.expr->expression, opts);

        case CONSTRAINT_GENERATED_IDENTITY:
            /* Compare GENERATED identity types */
            return c1->constraint.generated_identity.type == c2->constraint.generated_identity.type;

        case CONSTRAINT_GENERATED_ALWAYS:
            /* Compare GENERATED ALWAYS AS expressions */
            if (!c1->constraint.generated_always.expr || !c2->constraint.generated_always.expr) {
                return c1->constraint.generated_always.expr == c2->constraint.generated_always.expr;
            }
            return expressions_equal(c1->constraint.generated_always.expr->expression,
                                   c2->constraint.generated_always.expr->expression, opts);

        case CONSTRAINT_UNIQUE:
        case CONSTRAINT_PRIMARY_KEY:
            /* For column-level unique/pk, just type match */
            return true;

        case CONSTRAINT_REFERENCES:
            /* Compare foreign key details */
            if (!names_equal(c1->constraint.references.reftable,
                           c2->constraint.references.reftable, opts)) {
                return false;
            }
            /* TODO: Compare column mappings */
            return true;

        default:
            return false;
    }
}

/* Compare two table constraints for equivalence */
bool constraints_equivalent(const TableConstraint *c1, const TableConstraint *c2,
                           const CompareOptions *opts) {
    if (c1 == c2) {
        return true;
    }
    if (!c1 || !c2) {
        return false;
    }

    /* Must be same type */
    if (c1->type != c2->type) {
        return false;
    }

    /* If not ignoring names, names must match */
    if (opts && !opts->ignore_constraint_names) {
        if (!names_equal(c1->constraint_name, c2->constraint_name, opts)) {
            return false;
        }
    }

    switch (c1->type) {
        case TABLE_CONSTRAINT_CHECK:
            /* Compare CHECK expressions */
            if (!c1->constraint.check.expr || !c2->constraint.check.expr) {
                return c1->constraint.check.expr == c2->constraint.check.expr;
            }
            return expressions_equal(c1->constraint.check.expr->expression,
                                   c2->constraint.check.expr->expression, opts);

        case TABLE_CONSTRAINT_UNIQUE:
        case TABLE_CONSTRAINT_PRIMARY_KEY:
            /* TODO: Compare column lists */
            /* For now, just check if both exist */
            return true;

        case TABLE_CONSTRAINT_FOREIGN_KEY:
            /* TODO: Compare foreign key column mappings and references */
            return true;

        case TABLE_CONSTRAINT_EXCLUDE:
            /* TODO: Compare EXCLUDE elements */
            return true;

        case TABLE_CONSTRAINT_NOT_NULL:
            /* Compare column name */
            return names_equal(c1->constraint.not_null.column_name,
                             c2->constraint.not_null.column_name, opts);

        default:
            return false;
    }
}

/* Compare constraints between two tables */
void compare_constraints(const CreateTableStmt *source, const CreateTableStmt *target,
                        TableDiff *result, const CompareOptions *opts, MemoryContext *mem_ctx) {
    (void)mem_ctx;  /* Not used yet */

    if (!source || !target || !result) {
        return;
    }

    /* Build lists of table constraints */
    int source_count = 0;
    int target_count = 0;

    /* Count constraints */
    for (TableElement *elem = source->table_def.regular.elements; elem; elem = elem->next) {
        if (get_table_constraint(elem)) {
            source_count++;
        }
    }

    for (TableElement *elem = target->table_def.regular.elements; elem; elem = elem->next) {
        if (get_table_constraint(elem)) {
            target_count++;
        }
    }

    if (source_count == 0 && target_count == 0) {
        return;
    }

    /* Build arrays of constraints */
    const TableConstraint **source_constraints = NULL;
    const TableConstraint **target_constraints = NULL;

    if (source_count > 0) {
        source_constraints = malloc(sizeof(TableConstraint *) * source_count);
        if (!source_constraints) {
            return;
        }
        int idx = 0;
        for (TableElement *elem = source->table_def.regular.elements; elem; elem = elem->next) {
            const TableConstraint *tc = get_table_constraint(elem);
            if (tc) {
                source_constraints[idx++] = tc;
            }
        }
    }

    if (target_count > 0) {
        target_constraints = malloc(sizeof(TableConstraint *) * target_count);
        if (!target_constraints) {
            free(source_constraints);
            return;
        }
        int idx = 0;
        for (TableElement *elem = target->table_def.regular.elements; elem; elem = elem->next) {
            const TableConstraint *tc = get_table_constraint(elem);
            if (tc) {
                target_constraints[idx++] = tc;
            }
        }
    }

    /* Track which constraints have been matched */
    bool *source_matched = calloc(source_count, sizeof(bool));
    bool *target_matched = calloc(target_count, sizeof(bool));

    if (!source_matched || !target_matched) {
        free(source_constraints);
        free(target_constraints);
        free(source_matched);
        free(target_matched);
        return;
    }

    ConstraintDiff *last_added = NULL;
    ConstraintDiff *last_removed = NULL;

    /* Find matching constraints and modifications */
    for (int i = 0; i < target_count; i++) {
        const TableConstraint *target_c = target_constraints[i];
        bool found_match = false;

        for (int j = 0; j < source_count; j++) {
            if (source_matched[j]) {
                continue;
            }

            const TableConstraint *source_c = source_constraints[j];

            if (constraints_equivalent(source_c, target_c, opts)) {
                source_matched[j] = true;
                target_matched[i] = true;
                found_match = true;
                break;
            }
        }

        if (!found_match) {
            /* Constraint added */
            ConstraintDiff *cd = constraint_diff_create(target_c->constraint_name);
            if (cd) {
                cd->added = true;
                cd->new_type = target_c->type;
                result->constraint_add_count++;

                if (last_added) {
                    last_added->next = cd;
                } else {
                    result->constraints_added = cd;
                }
                last_added = cd;

                /* Create diff entry */
                Diff *diff = diff_create(DIFF_CONSTRAINT_ADDED, SEVERITY_INFO,
                                        result->table_name,
                                        target_c->constraint_name ? target_c->constraint_name : "(unnamed)");
                if (diff) {
                    const char *type_str = (target_c->type == TABLE_CONSTRAINT_CHECK) ? "CHECK" :
                                          (target_c->type == TABLE_CONSTRAINT_UNIQUE) ? "UNIQUE" :
                                          (target_c->type == TABLE_CONSTRAINT_PRIMARY_KEY) ? "PRIMARY KEY" :
                                          (target_c->type == TABLE_CONSTRAINT_FOREIGN_KEY) ? "FOREIGN KEY" :
                                          (target_c->type == TABLE_CONSTRAINT_EXCLUDE) ? "EXCLUDE" : "CONSTRAINT";
                    diff_set_values(diff, NULL, type_str);
                    diff_append(&result->diffs, diff);
                    result->diff_count++;
                }
            }
            target_matched[i] = true;
        }
    }

    /* Find removed constraints */
    for (int i = 0; i < source_count; i++) {
        if (!source_matched[i]) {
            const TableConstraint *source_c = source_constraints[i];

            ConstraintDiff *cd = constraint_diff_create(source_c->constraint_name);
            if (cd) {
                cd->removed = true;
                cd->old_type = source_c->type;
                result->constraint_remove_count++;

                if (last_removed) {
                    last_removed->next = cd;
                } else {
                    result->constraints_removed = cd;
                }
                last_removed = cd;

                /* Create diff entry */
                Diff *diff = diff_create(DIFF_CONSTRAINT_REMOVED, SEVERITY_WARNING,
                                        result->table_name,
                                        source_c->constraint_name ? source_c->constraint_name : "(unnamed)");
                if (diff) {
                    const char *type_str = (source_c->type == TABLE_CONSTRAINT_CHECK) ? "CHECK" :
                                          (source_c->type == TABLE_CONSTRAINT_UNIQUE) ? "UNIQUE" :
                                          (source_c->type == TABLE_CONSTRAINT_PRIMARY_KEY) ? "PRIMARY KEY" :
                                          (source_c->type == TABLE_CONSTRAINT_FOREIGN_KEY) ? "FOREIGN KEY" :
                                          (source_c->type == TABLE_CONSTRAINT_EXCLUDE) ? "EXCLUDE" : "CONSTRAINT";
                    diff_set_values(diff, type_str, NULL);
                    diff_append(&result->diffs, diff);
                    result->diff_count++;
                }
            }
        }
    }

    free(source_constraints);
    free(target_constraints);
    free(source_matched);
    free(target_matched);
}
