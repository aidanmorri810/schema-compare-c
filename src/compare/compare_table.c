#include "compare.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

/* Forward declarations */
void compare_columns(const CreateTableStmt *source, const CreateTableStmt *target,
                    TableDiff *result, const CompareOptions *opts, MemoryContext *mem_ctx);
void compare_constraints(const CreateTableStmt *source, const CreateTableStmt *target,
                        TableDiff *result, const CompareOptions *opts, MemoryContext *mem_ctx);

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
