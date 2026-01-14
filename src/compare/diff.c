#include "diff.h"
#include <stdlib.h>
#include <string.h>

// TODO: Convert this to using a memory context 

/* Create a new diff */
Diff *diff_create(DiffType type, DiffSeverity severity,
                  const char *table_name, const char *element_name) {
    Diff *diff = calloc(1, sizeof(Diff));
    if (!diff) {
        return NULL;
    }

    diff->type = type;
    diff->severity = severity;
    diff->table_name = table_name ? strdup(table_name) : NULL;
    diff->element_name = element_name ? strdup(element_name) : NULL;
    diff->old_value = NULL;
    diff->new_value = NULL;
    diff->description = NULL;
    diff->next = NULL;

    return diff;
}

/* Set values for a diff */
void diff_set_values(Diff *diff, const char *old_value, const char *new_value) {
    if (!diff) {
        return;
    }

    free(diff->old_value);
    free(diff->new_value);

    diff->old_value = old_value ? strdup(old_value) : NULL;
    diff->new_value = new_value ? strdup(new_value) : NULL;
}

/* Set description for a diff */
void diff_set_description(Diff *diff, const char *description) {
    if (!diff) {
        return;
    }

    free(diff->description);
    diff->description = description ? strdup(description) : NULL;
}

/* Append diff to list */
void diff_append(Diff **list, Diff *diff) {
    if (!list || !diff) {
        return;
    }

    if (!*list) {
        *list = diff;
        return;
    }

    Diff *tail = *list;
    while (tail->next) {
        tail = tail->next;
    }
    tail->next = diff;
}

/* Free a single diff */
void diff_free(Diff *diff) {
    if (!diff) {
        return;
    }

    free(diff->table_name);
    free(diff->element_name);
    free(diff->old_value);
    free(diff->new_value);
    free(diff->description);
    free(diff);
}

/* Free list of diffs */
void diff_list_free(Diff *list) {
    while (list) {
        Diff *next = list->next;
        diff_free(list);
        list = next;
    }
}

/* Create a ColumnDiff */
ColumnDiff *column_diff_create(const char *column_name) {
    ColumnDiff *cd = calloc(1, sizeof(ColumnDiff));
    if (!cd) {
        return NULL;
    }

    cd->column_name = column_name ? strdup(column_name) : NULL;
    cd->next = NULL;

    return cd;
}

/* Free a ColumnDiff */
void column_diff_free(ColumnDiff *cd) {
    if (!cd) {
        return;
    }

    free(cd->column_name);
    /* Note: old_type, new_type, etc. are pointers to original strings, not owned */
    free(cd);
}

/* Free list of ColumnDiffs */
void column_diff_list_free(ColumnDiff *list) {
    while (list) {
        ColumnDiff *next = list->next;
        column_diff_free(list);
        list = next;
    }
}

/* Create a ConstraintDiff */
ConstraintDiff *constraint_diff_create(const char *constraint_name) {
    ConstraintDiff *cd = calloc(1, sizeof(ConstraintDiff));
    if (!cd) {
        return NULL;
    }

    cd->constraint_name = constraint_name ? strdup(constraint_name) : NULL;
    cd->next = NULL;

    return cd;
}

/* Free a ConstraintDiff */
void constraint_diff_free(ConstraintDiff *cd) {
    if (!cd) {
        return;
    }

    free(cd->constraint_name);
    free(cd->old_definition);
    free(cd->new_definition);
    free(cd);
}

/* Free list of ConstraintDiffs */
void constraint_diff_list_free(ConstraintDiff *list) {
    while (list) {
        ConstraintDiff *next = list->next;
        constraint_diff_free(list);
        list = next;
    }
}

/* Create a TableDiff */
TableDiff *table_diff_create(const char *table_name) {
    TableDiff *td = calloc(1, sizeof(TableDiff));
    if (!td) {
        return NULL;
    }

    td->table_name = table_name ? strdup(table_name) : NULL;
    td->next = NULL;

    return td;
}

/* Free a TableDiff */
void table_diff_free(TableDiff *td) {
    if (!td) {
        return;
    }

    free(td->table_name);
    /* Note: old_tablespace, new_tablespace are pointers to original strings */

    column_diff_list_free(td->columns_added);
    column_diff_list_free(td->columns_removed);
    column_diff_list_free(td->columns_modified);

    constraint_diff_list_free(td->constraints_added);
    constraint_diff_list_free(td->constraints_removed);
    constraint_diff_list_free(td->constraints_modified);

    diff_list_free(td->diffs);

    free(td);
}

/* Free list of TableDiffs */
void table_diff_list_free(TableDiff *list) {
    while (list) {
        TableDiff *next = list->next;
        table_diff_free(list);
        list = next;
    }
}

/* Create a SchemaDiff */
SchemaDiff *schema_diff_create(const char *schema_name) {
    SchemaDiff *sd = calloc(1, sizeof(SchemaDiff));
    if (!sd) {
        return NULL;
    }

    sd->schema_name = schema_name ? strdup(schema_name) : NULL;

    return sd;
}

/* Free a SchemaDiff */
void schema_diff_free(SchemaDiff *sd) {
    if (!sd) {
        return;
    }

    free(sd->schema_name);

    table_diff_list_free(sd->table_diffs);

    for (int i = 0; i < sd->added_table_count; i++) {
        free(sd->added_tables[i]);
    }
    free(sd->added_tables);

    for (int i = 0; i < sd->removed_table_count; i++) {
        free(sd->removed_tables[i]);
    }
    free(sd->removed_tables);

    free(sd);
}

/* Convert DiffType to string */
const char *diff_type_to_string(DiffType type) {
    switch (type) {
        case DIFF_TABLE_ADDED: return "Table Added";
        case DIFF_TABLE_REMOVED: return "Table Removed";
        case DIFF_TABLE_MODIFIED: return "Table Modified";
        case DIFF_COLUMN_ADDED: return "Column Added";
        case DIFF_COLUMN_REMOVED: return "Column Removed";
        case DIFF_COLUMN_TYPE_CHANGED: return "Column Type Changed";
        case DIFF_COLUMN_NULLABLE_CHANGED: return "Column Nullable Changed";
        case DIFF_COLUMN_DEFAULT_CHANGED: return "Column Default Changed";
        case DIFF_COLUMN_COLLATION_CHANGED: return "Column Collation Changed";
        case DIFF_COLUMN_STORAGE_CHANGED: return "Column Storage Changed";
        case DIFF_COLUMN_COMPRESSION_CHANGED: return "Column Compression Changed";
        case DIFF_CONSTRAINT_ADDED: return "Constraint Added";
        case DIFF_CONSTRAINT_REMOVED: return "Constraint Removed";
        case DIFF_CONSTRAINT_MODIFIED: return "Constraint Modified";
        case DIFF_TABLE_TYPE_CHANGED: return "Table Type Changed";
        case DIFF_TABLESPACE_CHANGED: return "Tablespace Changed";
        case DIFF_PARTITION_CHANGED: return "Partition Changed";
        case DIFF_INHERITS_CHANGED: return "Inherits Changed";
        case DIFF_STORAGE_PARAMS_CHANGED: return "Storage Parameters Changed";
        default: return "Unknown";
    }
}

/* Convert DiffSeverity to string */
const char *diff_severity_to_string(DiffSeverity severity) {
    switch (severity) {
        case SEVERITY_INFO: return "INFO";
        case SEVERITY_WARNING: return "WARNING";
        case SEVERITY_CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

/* Determine severity from diff type */
DiffSeverity diff_determine_severity(DiffType type) {
    switch (type) {
        case DIFF_TABLE_REMOVED:
        case DIFF_COLUMN_REMOVED:
        case DIFF_COLUMN_TYPE_CHANGED:
        case DIFF_TABLE_TYPE_CHANGED:
            return SEVERITY_CRITICAL;

        case DIFF_TABLE_ADDED:
        case DIFF_COLUMN_ADDED:
        case DIFF_COLUMN_NULLABLE_CHANGED:
        case DIFF_CONSTRAINT_REMOVED:
            return SEVERITY_WARNING;

        case DIFF_COLUMN_DEFAULT_CHANGED:
        case DIFF_COLUMN_COLLATION_CHANGED:
        case DIFF_COLUMN_STORAGE_CHANGED:
        case DIFF_COLUMN_COMPRESSION_CHANGED:
        case DIFF_CONSTRAINT_ADDED:
        case DIFF_CONSTRAINT_MODIFIED:
        case DIFF_TABLESPACE_CHANGED:
        case DIFF_PARTITION_CHANGED:
        case DIFF_INHERITS_CHANGED:
        case DIFF_STORAGE_PARAMS_CHANGED:
        case DIFF_TABLE_MODIFIED:
            return SEVERITY_INFO;

        default:
            return SEVERITY_INFO;
    }
}
