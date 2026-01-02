#ifndef DIFF_H
#define DIFF_H

#include "pg_create_table.h"
#include <stdbool.h>

/* Difference types */
typedef enum {
    DIFF_TABLE_ADDED,
    DIFF_TABLE_REMOVED,
    DIFF_TABLE_MODIFIED,

    DIFF_COLUMN_ADDED,
    DIFF_COLUMN_REMOVED,
    DIFF_COLUMN_TYPE_CHANGED,
    DIFF_COLUMN_NULLABLE_CHANGED,
    DIFF_COLUMN_DEFAULT_CHANGED,
    DIFF_COLUMN_COLLATION_CHANGED,
    DIFF_COLUMN_STORAGE_CHANGED,
    DIFF_COLUMN_COMPRESSION_CHANGED,

    DIFF_CONSTRAINT_ADDED,
    DIFF_CONSTRAINT_REMOVED,
    DIFF_CONSTRAINT_MODIFIED,

    DIFF_TABLE_TYPE_CHANGED,
    DIFF_TABLESPACE_CHANGED,
    DIFF_PARTITION_CHANGED,
    DIFF_INHERITS_CHANGED,
    DIFF_STORAGE_PARAMS_CHANGED
} DiffType;

/* Severity levels */
typedef enum {
    SEVERITY_INFO,       /* Informational - non-breaking change */
    SEVERITY_WARNING,    /* Warning - may need attention */
    SEVERITY_CRITICAL    /* Critical - breaking change */
} DiffSeverity;

/* Generic difference structure */
typedef struct Diff {
    DiffType type;
    DiffSeverity severity;
    char *table_name;    /* Table this diff applies to */
    char *element_name;  /* Column/constraint name (NULL for table-level diffs) */
    char *old_value;     /* Previous value (NULL for additions) */
    char *new_value;     /* New value (NULL for removals) */
    char *description;   /* Human-readable description */
    struct Diff *next;   /* Linked list */
} Diff;

/* Column-level difference details */
typedef struct ColumnDiff {
    char *column_name;
    bool type_changed;
    bool nullable_changed;
    bool default_changed;
    bool collation_changed;
    bool storage_changed;
    bool compression_changed;

    char *old_type;
    char *new_type;
    bool old_nullable;
    bool new_nullable;
    char *old_default;
    char *new_default;
    char *old_collation;
    char *new_collation;
    char *old_storage;
    char *new_storage;
    char *old_compression;
    char *new_compression;

    struct ColumnDiff *next;
} ColumnDiff;

/* Constraint difference details */
typedef struct ConstraintDiff {
    char *constraint_name;
    bool added;
    bool removed;
    bool modified;

    /* Use int to hold either ConstraintType or TableConstraintType */
    int old_type;
    int new_type;
    char *old_definition;
    char *new_definition;

    struct ConstraintDiff *next;
} ConstraintDiff;

/* Table-level difference aggregation */
typedef struct TableDiff {
    char *table_name;
    bool table_added;
    bool table_removed;
    bool table_modified;

    /* Table-level changes */
    bool type_changed;        /* TEMPORARY, UNLOGGED, etc. */
    bool tablespace_changed;
    bool partition_changed;
    bool inherits_changed;
    bool storage_params_changed;

    TableType old_table_type;
    TableType new_table_type;
    char *old_tablespace;
    char *new_tablespace;

    /* Table definitions for added/removed tables and SQL generation */
    CreateTableStmt *source_table;  /* NULL if table was added */
    CreateTableStmt *target_table;  /* NULL if table was removed */

    /* Detailed column differences */
    ColumnDiff *columns_added;
    ColumnDiff *columns_removed;
    ColumnDiff *columns_modified;
    int column_add_count;
    int column_remove_count;
    int column_modify_count;

    /* Detailed constraint differences */
    ConstraintDiff *constraints_added;
    ConstraintDiff *constraints_removed;
    ConstraintDiff *constraints_modified;
    int constraint_add_count;
    int constraint_remove_count;
    int constraint_modify_count;

    /* Generic diff list for all changes */
    Diff *diffs;
    int diff_count;

    struct TableDiff *next;
} TableDiff;

/* Schema-level comparison results */
typedef struct SchemaDiff {
    char *schema_name;

    /* Summary counts */
    int tables_added;
    int tables_removed;
    int tables_modified;
    int total_diffs;

    /* Severity breakdown */
    int critical_count;
    int warning_count;
    int info_count;

    /* Detailed table differences */
    TableDiff *table_diffs;

    /* Quick lookup: added/removed table names */
    char **added_tables;
    char **removed_tables;
    int added_table_count;
    int removed_table_count;
} SchemaDiff;

/* Diff creation functions */
Diff *diff_create(DiffType type, DiffSeverity severity,
                  const char *table_name, const char *element_name);
void diff_set_values(Diff *diff, const char *old_value, const char *new_value);
void diff_set_description(Diff *diff, const char *description);
void diff_append(Diff **list, Diff *diff);
void diff_free(Diff *diff);
void diff_list_free(Diff *list);

/* ColumnDiff creation */
ColumnDiff *column_diff_create(const char *column_name);
void column_diff_free(ColumnDiff *cd);
void column_diff_list_free(ColumnDiff *list);

/* ConstraintDiff creation */
ConstraintDiff *constraint_diff_create(const char *constraint_name);
void constraint_diff_free(ConstraintDiff *cd);
void constraint_diff_list_free(ConstraintDiff *list);

/* TableDiff creation */
TableDiff *table_diff_create(const char *table_name);
void table_diff_free(TableDiff *td);
void table_diff_list_free(TableDiff *list);

/* SchemaDiff creation */
SchemaDiff *schema_diff_create(const char *schema_name);
void schema_diff_free(SchemaDiff *sd);

/* Utility functions */
const char *diff_type_to_string(DiffType type);
const char *diff_severity_to_string(DiffSeverity severity);
DiffSeverity diff_determine_severity(DiffType type);

#endif /* DIFF_H */
