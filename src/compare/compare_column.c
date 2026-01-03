#include "compare.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

/* Helper to extract ColumnDef from TableElement */
static const ColumnDef *get_column_def(const TableElement *elem) {
    if (elem && elem->type == TABLE_ELEM_COLUMN) {
        return &elem->elem.column;
    }
    return NULL;
}

/* Helper to check if column has NOT NULL constraint */
static bool column_is_not_null(const ColumnDef *col) {
    if (!col) {
        return false;
    }

    for (ColumnConstraint *c = col->constraints; c; c = c->next) {
        if (c->type == CONSTRAINT_NOT_NULL) {
            return true;
        }
    }
    return false;
}

/* Helper to get DEFAULT expression from column */
static const char *get_column_default(const ColumnDef *col) {
    if (!col) {
        return NULL;
    }

    for (ColumnConstraint *c = col->constraints; c; c = c->next) {
        if (c->type == CONSTRAINT_DEFAULT && c->constraint.default_val.expr) {
            return c->constraint.default_val.expr->expression;
        }
    }
    return NULL;
}

/* Compare two columns in detail */
ColumnDiff *compare_column_details(const ColumnDef *source, const ColumnDef *target,
                                  const CompareOptions *opts, MemoryContext *mem_ctx) {
    (void)mem_ctx;  /* Not used yet */

    if (!source || !target) {
        return NULL;
    }

    ColumnDiff *cd = column_diff_create(target->column_name);
    if (!cd) {
        return NULL;
    }

    bool has_changes = false;

    /* Compare data types */
    if (!data_types_equal(source->data_type, target->data_type, opts)) {
        cd->type_changed = true;
        cd->old_type = source->data_type;
        cd->new_type = target->data_type;
        has_changes = true;
    }

    /* Compare NOT NULL */
    bool source_not_null = column_is_not_null(source);
    bool target_not_null = column_is_not_null(target);
    if (source_not_null != target_not_null) {
        cd->nullable_changed = true;
        cd->old_nullable = !source_not_null;
        cd->new_nullable = !target_not_null;
        has_changes = true;
    }

    /* Compare DEFAULT */
    const char *source_default = get_column_default(source);
    const char *target_default = get_column_default(target);
    if (!expressions_equal(source_default, target_default, opts)) {
        cd->default_changed = true;
        cd->old_default = (char *)source_default;
        cd->new_default = (char *)target_default;
        has_changes = true;
    }

    /* Compare COLLATE */
    /* Only report collation change if both are explicitly set and different */
    /* Ignore "default" collation as it's the same as NULL */
    const char *src_collation = (source->collation && strcmp(source->collation, "default") == 0) ? NULL : source->collation;
    const char *tgt_collation = (target->collation && strcmp(target->collation, "default") == 0) ? NULL : target->collation;

    if (!names_equal(src_collation, tgt_collation, opts)) {
        /* Only report if there's a real difference (both non-NULL and different) */
        if (src_collation && tgt_collation) {
            cd->collation_changed = true;
            cd->old_collation = source->collation;
            cd->new_collation = target->collation;
            has_changes = true;
        }
    }

    /* Compare STORAGE */
    /* Only report storage change if explicitly different from type defaults */
    /* Database reader sets has_storage=true for all columns with their default storage,
     * but file definitions don't include STORAGE clause unless overriding the default.
     * We should only flag this as a change if the storage is explicitly different. */
    if (source->has_storage && target->has_storage) {
        /* Only report if storage types are explicitly different */
        if (source->storage_type != target->storage_type &&
            source->storage_type != STORAGE_TYPE_DEFAULT &&
            target->storage_type != STORAGE_TYPE_DEFAULT) {
            cd->storage_changed = true;
            const char *old_storage = (source->storage_type == STORAGE_TYPE_PLAIN) ? "PLAIN" :
                                     (source->storage_type == STORAGE_TYPE_EXTERNAL) ? "EXTERNAL" :
                                     (source->storage_type == STORAGE_TYPE_EXTENDED) ? "EXTENDED" :
                                     (source->storage_type == STORAGE_TYPE_MAIN) ? "MAIN" : "UNKNOWN";
            const char *new_storage = (target->storage_type == STORAGE_TYPE_PLAIN) ? "PLAIN" :
                                     (target->storage_type == STORAGE_TYPE_EXTERNAL) ? "EXTERNAL" :
                                     (target->storage_type == STORAGE_TYPE_EXTENDED) ? "EXTENDED" :
                                     (target->storage_type == STORAGE_TYPE_MAIN) ? "MAIN" : "UNKNOWN";
            cd->old_storage = (char *)old_storage;
            cd->new_storage = (char *)new_storage;
            has_changes = true;
        }
    }
    /* Don't report storage change if only one side has it set, as this is likely
     * just a difference between database introspection and file parsing */

    /* Compare COMPRESSION */
    if (!names_equal(source->compression_method, target->compression_method, opts)) {
        cd->compression_changed = true;
        cd->old_compression = source->compression_method;
        cd->new_compression = target->compression_method;
        has_changes = true;
    }

    if (!has_changes) {
        column_diff_free(cd);
        return NULL;
    }

    return cd;
}

/* Compare column lists */
void compare_columns(const CreateTableStmt *source, const CreateTableStmt *target,
                    TableDiff *result, const CompareOptions *opts, MemoryContext *mem_ctx) {
    if (!source || !target || !result) {
        return;
    }

    /* Build hash tables of columns by name */
    HashTable *source_ht = hash_table_create(32);
    HashTable *target_ht = hash_table_create(32);

    if (!source_ht || !target_ht) {
        hash_table_destroy(source_ht);
        hash_table_destroy(target_ht);
        return;
    }

    /* Populate source columns */
    for (TableElement *elem = source->table_def.regular.elements; elem; elem = elem->next) {
        const ColumnDef *col = get_column_def(elem);
        if (col && col->column_name) {
            hash_table_insert(source_ht, col->column_name, (void *)col);
        }
    }

    /* Populate target columns */
    for (TableElement *elem = target->table_def.regular.elements; elem; elem = elem->next) {
        const ColumnDef *col = get_column_def(elem);
        if (col && col->column_name) {
            hash_table_insert(target_ht, col->column_name, (void *)col);
        }
    }

    ColumnDiff *last_added = NULL;
    ColumnDiff *last_removed = NULL;
    ColumnDiff *last_modified = NULL;

    /* Find added and modified columns */
    for (TableElement *elem = target->table_def.regular.elements; elem; elem = elem->next) {
        const ColumnDef *target_col = get_column_def(elem);
        if (!target_col || !target_col->column_name) {
            continue;
        }

        const ColumnDef *source_col = hash_table_get(source_ht, target_col->column_name);

        if (!source_col) {
            /* Column added */
            ColumnDiff *cd = column_diff_create(target_col->column_name);
            if (cd) {
                cd->new_type = target_col->data_type;
                cd->new_nullable = !column_is_not_null(target_col);
                cd->new_default = (char *)get_column_default(target_col);
                result->column_add_count++;

                if (last_added) {
                    last_added->next = cd;
                } else {
                    result->columns_added = cd;
                }
                last_added = cd;

                /* Create diff entry */
                Diff *diff = diff_create(DIFF_COLUMN_ADDED, SEVERITY_WARNING,
                                        result->table_name, target_col->column_name);
                if (diff) {
                    diff_set_values(diff, NULL, target_col->data_type);
                    diff_append(&result->diffs, diff);
                    result->diff_count++;
                }
            }
        } else {
            /* Column exists - compare details */
            ColumnDiff *cd = compare_column_details(source_col, target_col, opts, mem_ctx);
            if (cd) {
                result->column_modify_count++;

                if (last_modified) {
                    last_modified->next = cd;
                } else {
                    result->columns_modified = cd;
                }
                last_modified = cd;

                /* Create diff entries for each change */
                if (cd->type_changed) {
                    Diff *diff = diff_create(DIFF_COLUMN_TYPE_CHANGED, SEVERITY_CRITICAL,
                                            result->table_name, cd->column_name);
                    if (diff) {
                        diff_set_values(diff, cd->old_type, cd->new_type);
                        diff_append(&result->diffs, diff);
                        result->diff_count++;
                    }
                }

                if (cd->nullable_changed) {
                    Diff *diff = diff_create(DIFF_COLUMN_NULLABLE_CHANGED, SEVERITY_WARNING,
                                            result->table_name, cd->column_name);
                    if (diff) {
                        diff_set_values(diff,
                                       cd->old_nullable ? "NULL" : "NOT NULL",
                                       cd->new_nullable ? "NULL" : "NOT NULL");
                        diff_append(&result->diffs, diff);
                        result->diff_count++;
                    }
                }

                if (cd->default_changed) {
                    Diff *diff = diff_create(DIFF_COLUMN_DEFAULT_CHANGED, SEVERITY_INFO,
                                            result->table_name, cd->column_name);
                    if (diff) {
                        diff_set_values(diff,
                                       cd->old_default ? cd->old_default : "(none)",
                                       cd->new_default ? cd->new_default : "(none)");
                        diff_append(&result->diffs, diff);
                        result->diff_count++;
                    }
                }

                if (cd->collation_changed) {
                    Diff *diff = diff_create(DIFF_COLUMN_COLLATION_CHANGED, SEVERITY_INFO,
                                            result->table_name, cd->column_name);
                    if (diff) {
                        diff_set_values(diff,
                                       cd->old_collation ? cd->old_collation : "(default)",
                                       cd->new_collation ? cd->new_collation : "(default)");
                        diff_append(&result->diffs, diff);
                        result->diff_count++;
                    }
                }

                if (cd->storage_changed) {
                    Diff *diff = diff_create(DIFF_COLUMN_STORAGE_CHANGED, SEVERITY_INFO,
                                            result->table_name, cd->column_name);
                    if (diff) {
                        diff_set_values(diff,
                                       cd->old_storage ? cd->old_storage : "(default)",
                                       cd->new_storage ? cd->new_storage : "(default)");
                        diff_append(&result->diffs, diff);
                        result->diff_count++;
                    }
                }

                if (cd->compression_changed) {
                    Diff *diff = diff_create(DIFF_COLUMN_COMPRESSION_CHANGED, SEVERITY_INFO,
                                            result->table_name, cd->column_name);
                    if (diff) {
                        diff_set_values(diff,
                                       cd->old_compression ? cd->old_compression : "(none)",
                                       cd->new_compression ? cd->new_compression : "(none)");
                        diff_append(&result->diffs, diff);
                        result->diff_count++;
                    }
                }
            }
        }
    }

    /* Find removed columns */
    for (TableElement *elem = source->table_def.regular.elements; elem; elem = elem->next) {
        const ColumnDef *source_col = get_column_def(elem);
        if (!source_col || !source_col->column_name) {
            continue;
        }

        const ColumnDef *target_col = hash_table_get(target_ht, source_col->column_name);

        if (!target_col) {
            /* Column removed */
            ColumnDiff *cd = column_diff_create(source_col->column_name);
            if (cd) {
                cd->old_type = source_col->data_type;
                result->column_remove_count++;

                if (last_removed) {
                    last_removed->next = cd;
                } else {
                    result->columns_removed = cd;
                }
                last_removed = cd;

                /* Create diff entry */
                Diff *diff = diff_create(DIFF_COLUMN_REMOVED, SEVERITY_CRITICAL,
                                        result->table_name, source_col->column_name);
                if (diff) {
                    diff_set_values(diff, source_col->data_type, NULL);
                    diff_append(&result->diffs, diff);
                    result->diff_count++;
                }
            }
        }
    }

    hash_table_destroy(source_ht);
    hash_table_destroy(target_ht);
}
