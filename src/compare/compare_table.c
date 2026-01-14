#include "compare.h"
#include "utils.h"
#include <string.h>

/* Compare all tables in a schema */
void compare_all_tables(CreateTableStmt **source_tables, int source_count,
                          CreateTableStmt **target_tables, int target_count,
                          SchemaDiff *result,
                          const CompareOptions *opts,
                          MemoryContext *mem_ctx) {
    if (!source_tables || !target_tables || !result) {
        return;
    }

    /* Build hash tables for O(1) lookup */
    HashTable *source_ht = hash_table_create(source_count * 2);
    HashTable *target_ht = hash_table_create(target_count * 2);

    if (!source_ht || !target_ht) {
        hash_table_destroy(source_ht);
        hash_table_destroy(target_ht);
        return;
    }

    /* Populate source hash table */
    for (int i = 0; i < source_count; i++) {
        if (source_tables[i] && source_tables[i]->table_name) {
            hash_table_insert(source_ht, source_tables[i]->table_name, source_tables[i]);
        }
    }

    /* Populate target hash table */
    for (int i = 0; i < target_count; i++) {
        if (target_tables[i] && target_tables[i]->table_name) {
            hash_table_insert(target_ht, target_tables[i]->table_name, target_tables[i]);
        }
    }

    TableDiff *last_diff = result->table_diffs;
    /* Find the end of existing table_diffs list */
    while (last_diff && last_diff->next) {
        last_diff = last_diff->next;
    }

    /* Find added and modified tables (tables in target but not in source, or different) */
    for (int i = 0; i < target_count; i++) {
        CreateTableStmt *target = target_tables[i];
        if (!target || !target->table_name) {
            continue;
        }

        if (!should_compare_table(target->table_name, opts)) {
            continue;
        }

        CreateTableStmt *source = hash_table_get(source_ht, target->table_name);

        if (!source) {
            /* Table added */
            TableDiff *diff = table_diff_create(target->table_name);
            if (diff) {
                diff->table_added = true;
                diff->target_table = target;  /* Store target table definition */
                diff->source_table = NULL;
                result->tables_added++;

                /* Link into list */
                if (last_diff) {
                    last_diff->next = diff;
                } else {
                    result->table_diffs = diff;
                }
                last_diff = diff;
            }
        } else {
            /* Table exists in both - compare details */
            TableDiff *diff = compare_tables(source, target, opts, mem_ctx);
            if (diff && diff->table_modified) {
                result->tables_modified++;
                result->total_diffs += diff->diff_count;

                /* Link into list */
                if (last_diff) {
                    last_diff->next = diff;
                } else {
                    result->table_diffs = diff;
                }
                last_diff = diff;
            }
        }
    }

    /* Find removed tables (tables in source but not in target) */
    for (int i = 0; i < source_count; i++) {
        CreateTableStmt *source = source_tables[i];
        if (!source || !source->table_name) {
            continue;
        }

        if (!should_compare_table(source->table_name, opts)) {
            continue;
        }

        CreateTableStmt *target = hash_table_get(target_ht, source->table_name);

        if (!target) {
            /* Table removed */
            TableDiff *diff = table_diff_create(source->table_name);
            if (diff) {
                diff->table_removed = true;
                diff->source_table = source;  /* Store source table definition */
                diff->target_table = NULL;
                result->tables_removed++;

                /* Link into list */
                if (last_diff) {
                    last_diff->next = diff;
                } else {
                    result->table_diffs = diff;
                }
                last_diff = diff;
            }
        }
    }

    /* Calculate severity counts */
    for (TableDiff *td = result->table_diffs; td; td = td->next) {
        for (Diff *d = td->diffs; d; d = d->next) {
            switch (d->severity) {
                case SEVERITY_CRITICAL:
                    result->critical_count++;
                    break;
                case SEVERITY_WARNING:
                    result->warning_count++;
                    break;
                case SEVERITY_INFO:
                    result->info_count++;
                    break;
            }
        }
    }

    hash_table_destroy(source_ht);
    hash_table_destroy(target_ht);
}

/* Compare two tables */
TableDiff *compare_tables(const CreateTableStmt *source, const CreateTableStmt *target,
                         const CompareOptions *opts, MemoryContext *mem_ctx) {
    if (!source || !target || !source->table_name || !target->table_name) {
        return NULL;
    }

    TableDiff *result = table_diff_create(target->table_name);
    if (!result) {
        return NULL;
    }

    /* Store table definitions for SQL generation */
    result->source_table = (CreateTableStmt *)source;
    result->target_table = (CreateTableStmt *)target;

    bool has_changes = false;

    /* Compare table types (TEMPORARY, UNLOGGED, etc.) */
    if (source->table_type != target->table_type) {
        result->type_changed = true;
        result->old_table_type = source->table_type;
        result->new_table_type = target->table_type;
        has_changes = true;

        Diff *diff = diff_create(DIFF_TABLE_TYPE_CHANGED, SEVERITY_CRITICAL,
                                target->table_name, NULL);
        if (diff) {
            const char *old_type_str = (source->table_type == TABLE_TYPE_TEMPORARY) ? "TEMPORARY" :
                                      (source->table_type == TABLE_TYPE_UNLOGGED) ? "UNLOGGED" : "NORMAL";
            const char *new_type_str = (target->table_type == TABLE_TYPE_TEMPORARY) ? "TEMPORARY" :
                                      (target->table_type == TABLE_TYPE_UNLOGGED) ? "UNLOGGED" : "NORMAL";
            diff_set_values(diff, old_type_str, new_type_str);
            diff_append(&result->diffs, diff);
            result->diff_count++;
        }
    }

    /* Compare tablespaces */
    if (opts && opts->compare_tablespaces) {
        bool tablespace_changed = false;
        if (source->tablespace_name && target->tablespace_name) {
            if (strcmp(source->tablespace_name, target->tablespace_name) != 0) {
                tablespace_changed = true;
            }
        } else if (source->tablespace_name || target->tablespace_name) {
            tablespace_changed = true;
        }

        if (tablespace_changed) {
            result->tablespace_changed = true;
            result->old_tablespace = source->tablespace_name;
            result->new_tablespace = target->tablespace_name;
            has_changes = true;

            Diff *diff = diff_create(DIFF_TABLESPACE_CHANGED, SEVERITY_INFO,
                                    target->table_name, NULL);
            if (diff) {
                diff_set_values(diff, source->tablespace_name ? source->tablespace_name : "(default)",
                               target->tablespace_name ? target->tablespace_name : "(default)");
                diff_append(&result->diffs, diff);
                result->diff_count++;
            }
        }
    }

    /* Compare columns */
    if (source->variant == CREATE_TABLE_REGULAR && target->variant == CREATE_TABLE_REGULAR) {
        compare_columns(source, target, result, opts, mem_ctx);
        if (result->column_add_count > 0 || result->column_remove_count > 0 ||
            result->column_modify_count > 0) {
            has_changes = true;
        }
    }

    /* Compare constraints */
    if (opts && opts->compare_constraints) {
        if (source->variant == CREATE_TABLE_REGULAR && target->variant == CREATE_TABLE_REGULAR) {
            compare_constraints(source, target, result, opts, mem_ctx);
            if (result->constraint_add_count > 0 || result->constraint_remove_count > 0 ||
                result->constraint_modify_count > 0) {
                has_changes = true;
            }
        }
    }

    /* TODO: Compare partitioning */
    if (opts && opts->compare_partitioning) {
        /* Stub for now */
    }

    /* TODO: Compare inheritance */
    if (opts && opts->compare_inheritance) {
        /* Stub for now */
    }

    result->table_modified = has_changes;

    return result;
}
