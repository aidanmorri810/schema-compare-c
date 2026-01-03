#ifndef SQL_GENERATOR_H
#define SQL_GENERATOR_H

#include "diff.h"
#include <stdbool.h>

/* SQL generation options */
typedef struct {
    bool use_transactions;       /* Wrap in BEGIN/COMMIT */
    bool use_if_exists;          /* Add IF EXISTS to DROP statements */
    bool add_comments;           /* Add explanatory comments */
    bool add_warnings;           /* Add warning comments for destructive ops */
    bool generate_rollback;      /* Generate rollback SQL (future) */
    bool safe_mode;              /* Extra safety checks */
    const char *schema_name;     /* Schema name for qualified identifiers */
} SQLGenOptions;

/* SQL migration script */
typedef struct {
    char *forward_sql;   /* Migration SQL to apply changes */
    char *rollback_sql;  /* Rollback SQL to undo changes (optional) */
    int statement_count; /* Number of SQL statements */
    bool has_destructive_changes;  /* Contains DROP or data-loss operations */
} SQLMigration;

/* ========== ORCHESTRATION (sql_generator.c) ========== */

/* Initialize default SQL generation options */
SQLGenOptions *sql_gen_options_default(void);
void sql_gen_options_free(SQLGenOptions *opts);

/* Generate migration SQL from diff */
SQLMigration *generate_migration_sql(const SchemaDiff *diff, const SQLGenOptions *opts);
void sql_migration_free(SQLMigration *migration);

/* Write migration to file */
bool write_migration_to_file(const SQLMigration *migration, const char *filename);

/* ========== TABLE OPERATIONS (sql_generator_table.c) ========== */

char *generate_create_table_sql(const CreateTableStmt *stmt, const SQLGenOptions *opts);
char *generate_drop_table_sql(const char *table_name, const SQLGenOptions *opts);

/* ========== COLUMN OPERATIONS (sql_generator_column.c) ========== */

char *generate_add_column_sql(const char *table_name, const ColumnDiff *col,
                              const SQLGenOptions *opts);
char *generate_drop_column_sql(const char *table_name, const char *column_name,
                               const SQLGenOptions *opts);
char *generate_alter_column_type_sql(const char *table_name, const ColumnDiff *col,
                                     const SQLGenOptions *opts);
char *generate_alter_column_nullable_sql(const char *table_name, const ColumnDiff *col,
                                         const SQLGenOptions *opts);
char *generate_alter_column_default_sql(const char *table_name, const ColumnDiff *col,
                                        const SQLGenOptions *opts);

/* ========== CONSTRAINT OPERATIONS (sql_generator_constraint.c) ========== */

char *generate_add_constraint_sql(const char *table_name, const ConstraintDiff *constraint,
                                  const SQLGenOptions *opts);
char *generate_drop_constraint_sql(const char *table_name, const char *constraint_name,
                                   const SQLGenOptions *opts);

/* ========== UTILITIES (sql_generator_util.c) ========== */

char *quote_identifier(const char *identifier);
char *quote_literal(const char *literal);
char *format_data_type(const char *type);

#endif /* SQL_GENERATOR_H */
