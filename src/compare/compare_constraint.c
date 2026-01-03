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

        case CONSTRAINT_REFERENCES: {
            /* Compare foreign key details */
            const ReferencesConstraint *ref1 = &c1->constraint.references;
            const ReferencesConstraint *ref2 = &c2->constraint.references;

            /* Compare referenced table */
            if (!names_equal(ref1->reftable, ref2->reftable, opts)) {
                return false;
            }

            /* Compare referenced column */
            if (!names_equal(ref1->refcolumn, ref2->refcolumn, opts)) {
                return false;
            }

            /* Compare match type if specified */
            if (ref1->has_match_type || ref2->has_match_type) {
                if (ref1->has_match_type != ref2->has_match_type) {
                    return false;
                }
                if (ref1->match_type != ref2->match_type) {
                    return false;
                }
            }

            /* Compare ON DELETE action */
            if (ref1->has_on_delete || ref2->has_on_delete) {
                if (ref1->has_on_delete != ref2->has_on_delete) {
                    return false;
                }
                if (ref1->on_delete != ref2->on_delete) {
                    return false;
                }
            }

            /* Compare ON UPDATE action */
            if (ref1->has_on_update || ref2->has_on_update) {
                if (ref1->has_on_update != ref2->has_on_update) {
                    return false;
                }
                if (ref1->on_update != ref2->on_update) {
                    return false;
                }
            }

            return true;
        }

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

        case TABLE_CONSTRAINT_UNIQUE: {
            const TableUniqueConstraint *uniq1 = &c1->constraint.unique;
            const TableUniqueConstraint *uniq2 = &c2->constraint.unique;

            /* Compare column counts */
            if (uniq1->column_count != uniq2->column_count) {
                return false;
            }

            /* Compare columns */
            for (int i = 0; i < uniq1->column_count; i++) {
                if (!names_equal(uniq1->columns[i], uniq2->columns[i], opts)) {
                    return false;
                }
            }

            /* Compare without overlaps column */
            if (!names_equal(uniq1->without_overlaps_column, uniq2->without_overlaps_column, opts)) {
                return false;
            }

            /* Compare NULLS DISTINCT if specified */
            if (uniq1->has_nulls_distinct || uniq2->has_nulls_distinct) {
                if (uniq1->has_nulls_distinct != uniq2->has_nulls_distinct) {
                    return false;
                }
                if (uniq1->nulls_distinct != uniq2->nulls_distinct) {
                    return false;
                }
            }

            /* Note: We don't compare index_params (storage options) as they're typically
             * considered implementation details rather than logical constraint differences */
            return true;
        }

        case TABLE_CONSTRAINT_PRIMARY_KEY: {
            const TablePrimaryKeyConstraint *pk1 = &c1->constraint.primary_key;
            const TablePrimaryKeyConstraint *pk2 = &c2->constraint.primary_key;

            /* Compare column counts */
            if (pk1->column_count != pk2->column_count) {
                return false;
            }

            /* Compare columns */
            for (int i = 0; i < pk1->column_count; i++) {
                if (!names_equal(pk1->columns[i], pk2->columns[i], opts)) {
                    return false;
                }
            }

            /* Compare without overlaps column */
            if (!names_equal(pk1->without_overlaps_column, pk2->without_overlaps_column, opts)) {
                return false;
            }

            /* Note: We don't compare index_params (storage options) as they're typically
             * considered implementation details rather than logical constraint differences */
            return true;
        }

        case TABLE_CONSTRAINT_FOREIGN_KEY: {
            const ForeignKeyConstraint *fk1 = &c1->constraint.foreign_key;
            const ForeignKeyConstraint *fk2 = &c2->constraint.foreign_key;

            /* Compare referenced table */
            if (!names_equal(fk1->reftable, fk2->reftable, opts)) {
                return false;
            }

            /* Compare column counts */
            if (fk1->column_count != fk2->column_count) {
                return false;
            }

            /* Compare local columns */
            for (int i = 0; i < fk1->column_count; i++) {
                if (!names_equal(fk1->columns[i], fk2->columns[i], opts)) {
                    return false;
                }
            }

            /* Compare referenced column counts */
            if (fk1->refcolumn_count != fk2->refcolumn_count) {
                return false;
            }

            /* Compare referenced columns */
            for (int i = 0; i < fk1->refcolumn_count; i++) {
                if (!names_equal(fk1->refcolumns[i], fk2->refcolumns[i], opts)) {
                    return false;
                }
            }

            /* Compare period columns */
            if (!names_equal(fk1->period_column, fk2->period_column, opts)) {
                return false;
            }

            if (!names_equal(fk1->ref_period_column, fk2->ref_period_column, opts)) {
                return false;
            }

            /* Compare match type if specified */
            if (fk1->has_match_type || fk2->has_match_type) {
                if (fk1->has_match_type != fk2->has_match_type) {
                    return false;
                }
                if (fk1->match_type != fk2->match_type) {
                    return false;
                }
            }

            /* Compare ON DELETE action */
            if (fk1->has_on_delete || fk2->has_on_delete) {
                if (fk1->has_on_delete != fk2->has_on_delete) {
                    return false;
                }
                if (fk1->on_delete != fk2->on_delete) {
                    return false;
                }
            }

            /* Compare ON UPDATE action */
            if (fk1->has_on_update || fk2->has_on_update) {
                if (fk1->has_on_update != fk2->has_on_update) {
                    return false;
                }
                if (fk1->on_update != fk2->on_update) {
                    return false;
                }
            }

            /* Compare ON DELETE SET column counts */
            if (fk1->on_delete_column_count != fk2->on_delete_column_count) {
                return false;
            }

            /* Compare ON DELETE SET columns */
            for (int i = 0; i < fk1->on_delete_column_count; i++) {
                if (!names_equal(fk1->on_delete_columns[i], fk2->on_delete_columns[i], opts)) {
                    return false;
                }
            }

            /* Compare ON UPDATE SET column counts */
            if (fk1->on_update_column_count != fk2->on_update_column_count) {
                return false;
            }

            /* Compare ON UPDATE SET columns */
            for (int i = 0; i < fk1->on_update_column_count; i++) {
                if (!names_equal(fk1->on_update_columns[i], fk2->on_update_columns[i], opts)) {
                    return false;
                }
            }

            return true;
        }

        case TABLE_CONSTRAINT_EXCLUDE: {
            const ExcludeConstraint *excl1 = &c1->constraint.exclude;
            const ExcludeConstraint *excl2 = &c2->constraint.exclude;

            /* Compare index method */
            if (!names_equal(excl1->index_method, excl2->index_method, opts)) {
                return false;
            }

            /* Compare element counts */
            if (excl1->element_count != excl2->element_count) {
                return false;
            }

            /* Compare each exclude element */
            for (int i = 0; i < excl1->element_count; i++) {
                const ExcludeElement *elem1 = &excl1->elements[i];
                const ExcludeElement *elem2 = &excl2->elements[i];

                /* Compare column name */
                if (!names_equal(elem1->column_name, elem2->column_name, opts)) {
                    return false;
                }

                /* Compare expression if present */
                if (elem1->expression || elem2->expression) {
                    if (!elem1->expression || !elem2->expression) {
                        return false;
                    }
                    if (!expressions_equal(elem1->expression->expression,
                                         elem2->expression->expression, opts)) {
                        return false;
                    }
                }

                /* Compare collation */
                if (!names_equal(elem1->collation, elem2->collation, opts)) {
                    return false;
                }

                /* Compare opclass if present */
                if (elem1->opclass || elem2->opclass) {
                    if (!elem1->opclass || !elem2->opclass) {
                        return false;
                    }
                    if (!names_equal(elem1->opclass->opclass, elem2->opclass->opclass, opts)) {
                        return false;
                    }
                    /* Note: We skip comparing opclass parameters as they're implementation details */
                }

                /* Compare sort order if specified */
                if (elem1->has_sort_order || elem2->has_sort_order) {
                    if (elem1->has_sort_order != elem2->has_sort_order) {
                        return false;
                    }
                    if (elem1->sort_order != elem2->sort_order) {
                        return false;
                    }
                }

                /* Compare nulls order if specified */
                if (elem1->has_nulls_order || elem2->has_nulls_order) {
                    if (elem1->has_nulls_order != elem2->has_nulls_order) {
                        return false;
                    }
                    if (elem1->nulls_order != elem2->nulls_order) {
                        return false;
                    }
                }

                /* Compare exclusion operator */
                if (excl1->operators && excl2->operators) {
                    if (!names_equal(excl1->operators[i], excl2->operators[i], opts)) {
                        return false;
                    }
                } else if (excl1->operators || excl2->operators) {
                    return false;
                }
            }

            /* Compare WHERE predicate if present */
            if (excl1->where_predicate || excl2->where_predicate) {
                if (!excl1->where_predicate || !excl2->where_predicate) {
                    return false;
                }
                if (!expressions_equal(excl1->where_predicate->expression,
                                     excl2->where_predicate->expression, opts)) {
                    return false;
                }
            }

            /* Note: We don't compare index_params (storage options) as they're typically
             * considered implementation details rather than logical constraint differences */
            return true;
        }

        case TABLE_CONSTRAINT_NOT_NULL:
            /* Compare column name */
            return names_equal(c1->constraint.not_null.column_name,
                             c2->constraint.not_null.column_name, opts);

        default:
            return false;
    }
}

/* Helper to get ColumnDef from TableElement */
static const ColumnDef *get_column_def(const TableElement *elem) {
    if (elem && elem->type == TABLE_ELEM_COLUMN) {
        return &elem->elem.column;
    }
    return NULL;
}

/* Structure to track constraint info (table-level or column-level) */
typedef struct {
    const TableConstraint *table_constraint;  /* NULL for column constraints */
    const ColumnConstraint *column_constraint;  /* NULL for table constraints */
    const char *column_name;  /* Set for column constraints */
    int constraint_type;  /* Unified type */
} ConstraintInfo;

/* Helper to compare two ConstraintInfo structures for equivalence */
static bool constraint_infos_equivalent(const ConstraintInfo *c1, const ConstraintInfo *c2,
                                       const CompareOptions *opts) {
    if (!c1 || !c2) {
        return false;
    }

    /* Must be same type */
    if (c1->constraint_type != c2->constraint_type) {
        return false;
    }

    /* If both are table-level constraints, use existing comparison */
    if (c1->table_constraint && c2->table_constraint) {
        return constraints_equivalent(c1->table_constraint, c2->table_constraint, opts);
    }

    /* If both are column-level constraints */
    if (c1->column_constraint && c2->column_constraint) {
        /* Must be on the same column */
        if (!names_equal(c1->column_name, c2->column_name, opts)) {
            return false;
        }

        /* For inline PK and UNIQUE, type match and same column is sufficient */
        if (c1->constraint_type == TABLE_CONSTRAINT_PRIMARY_KEY ||
            c1->constraint_type == TABLE_CONSTRAINT_UNIQUE) {
            return true;
        }
    }

    /* Mixed constraint types (one table-level, one column-level) - check if semantically equivalent */
    if ((c1->table_constraint && c2->column_constraint) ||
        (c1->column_constraint && c2->table_constraint)) {

        /* For PRIMARY KEY and UNIQUE, check if they apply to the same column */
        if (c1->constraint_type == TABLE_CONSTRAINT_PRIMARY_KEY) {
            const TablePrimaryKeyConstraint *pk = NULL;
            const char *col_name = NULL;

            if (c1->table_constraint) {
                pk = &c1->table_constraint->constraint.primary_key;
                col_name = c2->column_name;
            } else {
                pk = &c2->table_constraint->constraint.primary_key;
                col_name = c1->column_name;
            }

            /* Check if PK is single-column and matches the inline constraint column */
            if (pk->column_count == 1 && names_equal(pk->columns[0], col_name, opts)) {
                return true;
            }
        }

        if (c1->constraint_type == TABLE_CONSTRAINT_UNIQUE) {
            const TableUniqueConstraint *uniq = NULL;
            const char *col_name = NULL;

            if (c1->table_constraint) {
                uniq = &c1->table_constraint->constraint.unique;
                col_name = c2->column_name;
            } else {
                uniq = &c2->table_constraint->constraint.unique;
                col_name = c1->column_name;
            }

            /* Check if UNIQUE is single-column and matches the inline constraint column */
            if (uniq->column_count == 1 && names_equal(uniq->columns[0], col_name, opts)) {
                return true;
            }
        }
    }

    return false;
}

/* Helper to check if a set of column-level PK constraints matches a table-level PK */
static bool column_pks_match_table_pk(const ConstraintInfo *source_constraints, int source_count,
                                     const bool *source_matched,
                                     const TablePrimaryKeyConstraint *table_pk,
                                     const CompareOptions *opts,
                                     int *matched_indices, int *matched_count) {
    *matched_count = 0;

    /* Count unmatched column-level PRIMARY KEY constraints */
    int available_count = 0;
    for (int i = 0; i < source_count; i++) {
        if (!source_matched[i] &&
            source_constraints[i].column_constraint &&
            source_constraints[i].constraint_type == TABLE_CONSTRAINT_PRIMARY_KEY) {
            available_count++;
        }
    }

    /* Must have same number of columns */
    if (available_count != table_pk->column_count) {
        return false;
    }

    /* Check if all table PK columns have corresponding column-level PK constraints */
    for (int pk_col = 0; pk_col < table_pk->column_count; pk_col++) {
        bool found = false;

        for (int i = 0; i < source_count; i++) {
            if (source_matched[i]) {
                continue;
            }

            if (source_constraints[i].column_constraint &&
                source_constraints[i].constraint_type == TABLE_CONSTRAINT_PRIMARY_KEY &&
                names_equal(source_constraints[i].column_name, table_pk->columns[pk_col], opts)) {

                /* Found matching column */
                matched_indices[*matched_count] = i;
                (*matched_count)++;
                found = true;
                break;
            }
        }

        if (!found) {
            *matched_count = 0;
            return false;
        }
    }

    return true;
}

/* Helper to check if a set of column-level UNIQUE constraints matches a table-level UNIQUE */
static bool column_uniques_match_table_unique(const ConstraintInfo *source_constraints, int source_count,
                                              const bool *source_matched,
                                              const TableUniqueConstraint *table_uniq,
                                              const CompareOptions *opts,
                                              int *matched_indices, int *matched_count) {
    *matched_count = 0;

    /* Count unmatched column-level UNIQUE constraints */
    int available_count = 0;
    for (int i = 0; i < source_count; i++) {
        if (!source_matched[i] &&
            source_constraints[i].column_constraint &&
            source_constraints[i].constraint_type == TABLE_CONSTRAINT_UNIQUE) {
            available_count++;
        }
    }

    /* Must have same number of columns */
    if (available_count != table_uniq->column_count) {
        return false;
    }

    /* Check if all table UNIQUE columns have corresponding column-level UNIQUE constraints */
    for (int uniq_col = 0; uniq_col < table_uniq->column_count; uniq_col++) {
        bool found = false;

        for (int i = 0; i < source_count; i++) {
            if (source_matched[i]) {
                continue;
            }

            if (source_constraints[i].column_constraint &&
                source_constraints[i].constraint_type == TABLE_CONSTRAINT_UNIQUE &&
                names_equal(source_constraints[i].column_name, table_uniq->columns[uniq_col], opts)) {

                /* Found matching column */
                matched_indices[*matched_count] = i;
                (*matched_count)++;
                found = true;
                break;
            }
        }

        if (!found) {
            *matched_count = 0;
            return false;
        }
    }

    return true;
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

    /* Count table-level constraints */
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

    /* Also count inline PRIMARY KEY and UNIQUE constraints on columns */
    for (TableElement *elem = source->table_def.regular.elements; elem; elem = elem->next) {
        const ColumnDef *col = get_column_def(elem);
        if (col && col->constraints) {
            for (ColumnConstraint *cc = col->constraints; cc; cc = cc->next) {
                if (cc->type == CONSTRAINT_PRIMARY_KEY || cc->type == CONSTRAINT_UNIQUE) {
                    source_count++;
                }
            }
        }
    }

    for (TableElement *elem = target->table_def.regular.elements; elem; elem = elem->next) {
        const ColumnDef *col = get_column_def(elem);
        if (col && col->constraints) {
            for (ColumnConstraint *cc = col->constraints; cc; cc = cc->next) {
                if (cc->type == CONSTRAINT_PRIMARY_KEY || cc->type == CONSTRAINT_UNIQUE) {
                    target_count++;
                }
            }
        }
    }

    if (source_count == 0 && target_count == 0) {
        return;
    }

    /* Build arrays of constraints */
    ConstraintInfo *source_constraints = NULL;
    ConstraintInfo *target_constraints = NULL;

    if (source_count > 0) {
        source_constraints = calloc(source_count, sizeof(ConstraintInfo));
        if (!source_constraints) {
            return;
        }
        int idx = 0;

        /* Add table-level constraints */
        for (TableElement *elem = source->table_def.regular.elements; elem; elem = elem->next) {
            const TableConstraint *tc = get_table_constraint(elem);
            if (tc) {
                source_constraints[idx].table_constraint = tc;
                source_constraints[idx].constraint_type = tc->type;
                idx++;
            }
        }

        /* Add inline column constraints (PK and UNIQUE only) */
        for (TableElement *elem = source->table_def.regular.elements; elem; elem = elem->next) {
            const ColumnDef *col = get_column_def(elem);
            if (col && col->constraints) {
                for (ColumnConstraint *cc = col->constraints; cc; cc = cc->next) {
                    if (cc->type == CONSTRAINT_PRIMARY_KEY) {
                        source_constraints[idx].column_constraint = cc;
                        source_constraints[idx].column_name = col->column_name;
                        source_constraints[idx].constraint_type = TABLE_CONSTRAINT_PRIMARY_KEY;
                        idx++;
                    } else if (cc->type == CONSTRAINT_UNIQUE) {
                        source_constraints[idx].column_constraint = cc;
                        source_constraints[idx].column_name = col->column_name;
                        source_constraints[idx].constraint_type = TABLE_CONSTRAINT_UNIQUE;
                        idx++;
                    }
                }
            }
        }
    }

    if (target_count > 0) {
        target_constraints = calloc(target_count, sizeof(ConstraintInfo));
        if (!target_constraints) {
            free(source_constraints);
            return;
        }
        int idx = 0;

        /* Add table-level constraints */
        for (TableElement *elem = target->table_def.regular.elements; elem; elem = elem->next) {
            const TableConstraint *tc = get_table_constraint(elem);
            if (tc) {
                target_constraints[idx].table_constraint = tc;
                target_constraints[idx].constraint_type = tc->type;
                idx++;
            }
        }

        /* Add inline column constraints (PK and UNIQUE only) */
        for (TableElement *elem = target->table_def.regular.elements; elem; elem = elem->next) {
            const ColumnDef *col = get_column_def(elem);
            if (col && col->constraints) {
                for (ColumnConstraint *cc = col->constraints; cc; cc = cc->next) {
                    if (cc->type == CONSTRAINT_PRIMARY_KEY) {
                        target_constraints[idx].column_constraint = cc;
                        target_constraints[idx].column_name = col->column_name;
                        target_constraints[idx].constraint_type = TABLE_CONSTRAINT_PRIMARY_KEY;
                        idx++;
                    } else if (cc->type == CONSTRAINT_UNIQUE) {
                        target_constraints[idx].column_constraint = cc;
                        target_constraints[idx].column_name = col->column_name;
                        target_constraints[idx].constraint_type = TABLE_CONSTRAINT_UNIQUE;
                        idx++;
                    }
                }
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
        const ConstraintInfo *target_c = &target_constraints[i];
        bool found_match = false;

        /* First, try direct equivalence check */
        for (int j = 0; j < source_count; j++) {
            if (source_matched[j]) {
                continue;
            }

            const ConstraintInfo *source_c = &source_constraints[j];

            if (constraint_infos_equivalent(source_c, target_c, opts)) {
                source_matched[j] = true;
                target_matched[i] = true;
                found_match = true;
                break;
            }
        }

        /* If not found and target is a table-level PK/UNIQUE, check if it matches multiple column-level constraints */
        if (!found_match && target_c->table_constraint) {
            if (target_c->constraint_type == TABLE_CONSTRAINT_PRIMARY_KEY) {
                const TablePrimaryKeyConstraint *table_pk = &target_c->table_constraint->constraint.primary_key;
                int matched_indices[32]; /* Reasonable max for composite PK */
                int matched_count = 0;

                if (column_pks_match_table_pk(source_constraints, source_count, source_matched,
                                             table_pk, opts, matched_indices, &matched_count)) {
                    /* Mark all matched column-level PKs as matched */
                    for (int k = 0; k < matched_count; k++) {
                        source_matched[matched_indices[k]] = true;
                    }
                    target_matched[i] = true;
                    found_match = true;
                }
            } else if (target_c->constraint_type == TABLE_CONSTRAINT_UNIQUE) {
                const TableUniqueConstraint *table_uniq = &target_c->table_constraint->constraint.unique;
                int matched_indices[32];
                int matched_count = 0;

                if (column_uniques_match_table_unique(source_constraints, source_count, source_matched,
                                                     table_uniq, opts, matched_indices, &matched_count)) {
                    /* Mark all matched column-level UNIQUE constraints as matched */
                    for (int k = 0; k < matched_count; k++) {
                        source_matched[matched_indices[k]] = true;
                    }
                    target_matched[i] = true;
                    found_match = true;
                }
            }
        }

        if (!found_match) {
            /* Constraint added */
            const char *constraint_name = NULL;
            if (target_c->table_constraint && target_c->table_constraint->constraint_name) {
                constraint_name = target_c->table_constraint->constraint_name;
            } else if (target_c->column_name) {
                /* For inline constraints, use column name as identifier */
                constraint_name = target_c->column_name;
            }

            ConstraintDiff *cd = constraint_diff_create(constraint_name);
            if (cd) {
                cd->added = true;
                cd->new_type = target_c->constraint_type;

                /* Store constraint pointers for SQL generation */
                if (target_c->table_constraint) {
                    cd->target_constraint = target_c->table_constraint;
                    cd->is_column_constraint = false;
                } else if (target_c->column_constraint) {
                    cd->target_constraint = target_c->column_constraint;
                    cd->column_name = target_c->column_name;
                    cd->is_column_constraint = true;
                }

                result->constraint_add_count++;

                if (last_added) {
                    last_added->next = cd;
                } else {
                    result->constraints_added = cd;
                }
                last_added = cd;

                /* Create diff entry */
                const char *type_str = (target_c->constraint_type == TABLE_CONSTRAINT_CHECK) ? "CHECK" :
                                      (target_c->constraint_type == TABLE_CONSTRAINT_UNIQUE) ? "UNIQUE" :
                                      (target_c->constraint_type == TABLE_CONSTRAINT_PRIMARY_KEY) ? "PRIMARY KEY" :
                                      (target_c->constraint_type == TABLE_CONSTRAINT_FOREIGN_KEY) ? "FOREIGN KEY" :
                                      (target_c->constraint_type == TABLE_CONSTRAINT_EXCLUDE) ? "EXCLUDE" : "CONSTRAINT";

                Diff *diff = diff_create(DIFF_CONSTRAINT_ADDED, SEVERITY_INFO,
                                        result->table_name,
                                        constraint_name ? constraint_name : "(unnamed)");
                if (diff) {
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
            const ConstraintInfo *source_c = &source_constraints[i];
            bool found_reverse_match = false;

            /* Check reverse case: source table-level PK/UNIQUE matching target column-level constraints */
            if (source_c->table_constraint) {
                if (source_c->constraint_type == TABLE_CONSTRAINT_PRIMARY_KEY) {
                    const TablePrimaryKeyConstraint *table_pk = &source_c->table_constraint->constraint.primary_key;
                    int matched_indices[32];
                    int matched_count = 0;

                    if (column_pks_match_table_pk(target_constraints, target_count, target_matched,
                                                 table_pk, opts, matched_indices, &matched_count)) {
                        /* Mark all matched column-level PKs as matched */
                        for (int k = 0; k < matched_count; k++) {
                            target_matched[matched_indices[k]] = true;
                        }
                        source_matched[i] = true;
                        found_reverse_match = true;
                    }
                } else if (source_c->constraint_type == TABLE_CONSTRAINT_UNIQUE) {
                    const TableUniqueConstraint *table_uniq = &source_c->table_constraint->constraint.unique;
                    int matched_indices[32];
                    int matched_count = 0;

                    if (column_uniques_match_table_unique(target_constraints, target_count, target_matched,
                                                         table_uniq, opts, matched_indices, &matched_count)) {
                        /* Mark all matched column-level UNIQUE constraints as matched */
                        for (int k = 0; k < matched_count; k++) {
                            target_matched[matched_indices[k]] = true;
                        }
                        source_matched[i] = true;
                        found_reverse_match = true;
                    }
                }
            }

            if (found_reverse_match) {
                continue;
            }

            const char *constraint_name = NULL;
            if (source_c->table_constraint && source_c->table_constraint->constraint_name) {
                constraint_name = source_c->table_constraint->constraint_name;
            } else if (source_c->column_name) {
                /* For inline constraints, use column name as identifier */
                constraint_name = source_c->column_name;
            }

            ConstraintDiff *cd = constraint_diff_create(constraint_name);
            if (cd) {
                cd->removed = true;
                cd->old_type = source_c->constraint_type;

                /* Store constraint pointers for SQL generation */
                if (source_c->table_constraint) {
                    cd->source_constraint = source_c->table_constraint;
                    cd->is_column_constraint = false;
                } else if (source_c->column_constraint) {
                    cd->source_constraint = source_c->column_constraint;
                    cd->column_name = source_c->column_name;
                    cd->is_column_constraint = true;
                }

                result->constraint_remove_count++;

                if (last_removed) {
                    last_removed->next = cd;
                } else {
                    result->constraints_removed = cd;
                }
                last_removed = cd;

                /* Create diff entry */
                const char *type_str = (source_c->constraint_type == TABLE_CONSTRAINT_CHECK) ? "CHECK" :
                                      (source_c->constraint_type == TABLE_CONSTRAINT_UNIQUE) ? "UNIQUE" :
                                      (source_c->constraint_type == TABLE_CONSTRAINT_PRIMARY_KEY) ? "PRIMARY KEY" :
                                      (source_c->constraint_type == TABLE_CONSTRAINT_FOREIGN_KEY) ? "FOREIGN KEY" :
                                      (source_c->constraint_type == TABLE_CONSTRAINT_EXCLUDE) ? "EXCLUDE" : "CONSTRAINT";

                Diff *diff = diff_create(DIFF_CONSTRAINT_REMOVED, SEVERITY_WARNING,
                                        result->table_name,
                                        constraint_name ? constraint_name : "(unnamed)");
                if (diff) {
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
