#ifndef SC_MEMORY_H
#define SC_MEMORY_H

#include "pg_create_table.h"
#include <stddef.h>
#include <stdbool.h>

/* Memory context for tracking allocations */
typedef struct MemoryContext MemoryContext;

/* Memory management API */
MemoryContext *memory_context_create(const char *name);
void memory_context_destroy(MemoryContext *ctx);
void memory_context_reset(MemoryContext *ctx);

/* Allocation functions */
void *mem_alloc(MemoryContext *ctx, size_t size);
void *mem_calloc(MemoryContext *ctx, size_t nmemb, size_t size);
void *mem_realloc(MemoryContext *ctx, void *ptr, size_t size);
char *mem_strdup(MemoryContext *ctx, const char *str);
char *mem_strndup(MemoryContext *ctx, const char *str, size_t n);
void mem_free(MemoryContext *ctx, void *ptr);

/* CreateTableStmt construction helpers */
CreateTableStmt *create_table_stmt_alloc(MemoryContext *ctx);
ColumnDef *column_def_alloc(MemoryContext *ctx);
TableConstraint *table_constraint_alloc(MemoryContext *ctx);
ColumnConstraint *column_constraint_alloc(MemoryContext *ctx);
Expression *expression_alloc(MemoryContext *ctx, const char *expr_text);
TableElement *table_element_alloc(MemoryContext *ctx);
StorageParameter *storage_parameter_alloc(MemoryContext *ctx);

/* CreateTableStmt destruction */
void free_create_table_stmt(CreateTableStmt *stmt);
void free_column_def(ColumnDef *col);
void free_table_constraint(TableConstraint *constraint);
void free_column_constraint(ColumnConstraint *constraint);
void free_expression(Expression *expr);
void free_partition_by_clause(PartitionByClause *partition);
void free_storage_parameter_list(StorageParameterList *params);

/* Deep copy functions for comparison */
CreateTableStmt *clone_create_table_stmt(const CreateTableStmt *src, MemoryContext *ctx);
ColumnDef *clone_column_def(const ColumnDef *src, MemoryContext *ctx);

/* Memory statistics (for debugging) */
size_t memory_context_get_allocated(MemoryContext *ctx);
void memory_context_stats(MemoryContext *ctx);

#endif /* SC_MEMORY_H */
