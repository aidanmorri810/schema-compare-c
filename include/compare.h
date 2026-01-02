#ifndef COMPARE_H
#define COMPARE_H

#include "pg_create_table.h"
#include "diff.h"
#include "sc_memory.h"
#include <stdbool.h>

/* Comparison options */
typedef struct CompareOptions {
    /* What to compare */
    bool compare_tablespaces;      /* Default: false - ignore tablespace differences */
    bool compare_storage_params;   /* Default: true - compare storage parameters */
    bool compare_constraints;      /* Default: true - compare constraints */
    bool compare_partitioning;     /* Default: true - compare partition info */
    bool compare_inheritance;      /* Default: true - compare INHERITS */

    /* Comparison behavior */
    bool case_sensitive;           /* Default: false - case-insensitive names */
    bool normalize_types;          /* Default: true - normalize type names (int4 vs integer) */
    bool ignore_constraint_names;  /* Default: false - match constraints by semantics */
    bool ignore_whitespace;        /* Default: true - ignore whitespace in expressions */

    /* Filtering */
    char **exclude_tables;         /* NULL-terminated array of table patterns to exclude */
    char **include_tables;         /* NULL-terminated array of table patterns to include */
    int exclude_count;
    int include_count;
} CompareOptions;

/* Initialize default comparison options */
CompareOptions *compare_options_default(void);
void compare_options_free(CompareOptions *opts);

/* Main comparison functions */

/* Compare two schemas (arrays of CreateTableStmt) */
SchemaDiff *compare_schemas(CreateTableStmt **source_tables, int source_count,
                           CreateTableStmt **target_tables, int target_count,
                           const CompareOptions *opts,
                           MemoryContext *mem_ctx);

/* Compare two individual tables */
TableDiff *compare_tables(const CreateTableStmt *source, const CreateTableStmt *target,
                         const CompareOptions *opts,
                         MemoryContext *mem_ctx);

/* Compare column lists */
void compare_columns(const CreateTableStmt *source, const CreateTableStmt *target,
                    TableDiff *result,
                    const CompareOptions *opts,
                    MemoryContext *mem_ctx);

/* Compare individual columns */
ColumnDiff *compare_column_details(const ColumnDef *source, const ColumnDef *target,
                                  const CompareOptions *opts,
                                  MemoryContext *mem_ctx);

/* Compare constraints */
void compare_constraints(const CreateTableStmt *source, const CreateTableStmt *target,
                        TableDiff *result,
                        const CompareOptions *opts,
                        MemoryContext *mem_ctx);

/* Compare individual constraints */
bool constraints_equivalent(const TableConstraint *c1, const TableConstraint *c2,
                           const CompareOptions *opts);
bool column_constraints_equivalent(const ColumnConstraint *c1, const ColumnConstraint *c2,
                                  const CompareOptions *opts);

/* Utility comparison functions */

/* Check if table should be included in comparison based on filters */
bool should_compare_table(const char *table_name, const CompareOptions *opts);

/* Normalize data type names for comparison */
char *normalize_type_name(const char *type_name, MemoryContext *mem_ctx);

/* Compare data types accounting for normalization */
bool data_types_equal(const char *type1, const char *type2, const CompareOptions *opts);

/* Compare expressions accounting for whitespace */
bool expressions_equal(const char *expr1, const char *expr2, const CompareOptions *opts);

/* Case-sensitive or insensitive string comparison */
bool names_equal(const char *name1, const char *name2, const CompareOptions *opts);

#endif /* COMPARE_H */
