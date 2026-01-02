#include "sc_memory.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

/* Memory block tracking structure */
typedef struct MemBlock {
    void *ptr;
    size_t size;
    struct MemBlock *next;
} MemBlock;

/* Memory context implementation */
typedef struct MemoryContext {
    char *name;
    MemBlock *blocks;
    size_t total_allocated;
    size_t block_count;
} MemoryContext;

/* Forward declaration */
static void memory_context_reset_internal(MemoryContext *ctx);

/* Create a new memory context */
MemoryContext *memory_context_create(const char *name) {
    MemoryContext *ctx = malloc(sizeof(MemoryContext));
    if (!ctx) {
        return NULL;
    }

    ctx->name = name ? strdup(name) : strdup("unnamed");
    ctx->blocks = NULL;
    ctx->total_allocated = 0;
    ctx->block_count = 0;

    return ctx;
}

/* Reset context (free all blocks but keep context) */
static void memory_context_reset_internal(MemoryContext *ctx) {
    if (!ctx) {
        return;
    }

    MemBlock *block = ctx->blocks;
    while (block) {
        MemBlock *next = block->next;
        free(block->ptr);
        free(block);
        block = next;
    }

    ctx->blocks = NULL;
    ctx->total_allocated = 0;
    ctx->block_count = 0;
}

/* Destroy memory context and free all allocations */
void memory_context_destroy(MemoryContext *ctx) {
    if (!ctx) {
        return;
    }

    memory_context_reset_internal(ctx);
    free(ctx->name);
    free(ctx);
}

/* Reset context - public API */
void memory_context_reset(MemoryContext *ctx) {
    memory_context_reset_internal(ctx);
}

/* Internal function to track allocation */
static void track_allocation(MemoryContext *ctx, void *ptr, size_t size) {
    if (!ctx || !ptr) {
        return;
    }

    MemBlock *block = malloc(sizeof(MemBlock));
    if (!block) {
        return;
    }

    block->ptr = ptr;
    block->size = size;
    block->next = ctx->blocks;
    ctx->blocks = block;
    ctx->total_allocated += size;
    ctx->block_count++;
}

/* Internal function to untrack allocation */
static void untrack_allocation(MemoryContext *ctx, void *ptr) {
    if (!ctx || !ptr) {
        return;
    }

    MemBlock *prev = NULL;
    MemBlock *block = ctx->blocks;

    while (block) {
        if (block->ptr == ptr) {
            if (prev) {
                prev->next = block->next;
            } else {
                ctx->blocks = block->next;
            }

            ctx->total_allocated -= block->size;
            ctx->block_count--;
            free(block);
            return;
        }
        prev = block;
        block = block->next;
    }
}

/* Allocate memory in context */
void *mem_alloc(MemoryContext *ctx, size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        return NULL;
    }

    if (ctx) {
        track_allocation(ctx, ptr, size);
    }

    return ptr;
}

/* Allocate zero-initialized memory */
void *mem_calloc(MemoryContext *ctx, size_t nmemb, size_t size) {
    void *ptr = calloc(nmemb, size);
    if (!ptr) {
        return NULL;
    }

    if (ctx) {
        track_allocation(ctx, ptr, nmemb * size);
    }

    return ptr;
}

/* Reallocate memory */
void *mem_realloc(MemoryContext *ctx, void *ptr, size_t size) {
    if (!ptr) {
        return mem_alloc(ctx, size);
    }

    if (ctx) {
        untrack_allocation(ctx, ptr);
    }

    void *new_ptr = realloc(ptr, size);
    if (!new_ptr) {
        return NULL;
    }

    if (ctx) {
        track_allocation(ctx, new_ptr, size);
    }

    return new_ptr;
}

/* Duplicate string in context */
char *mem_strdup(MemoryContext *ctx, const char *str) {
    if (!str) {
        return NULL;
    }

    size_t len = strlen(str) + 1;
    char *copy = mem_alloc(ctx, len);
    if (!copy) {
        return NULL;
    }

    memcpy(copy, str, len);
    return copy;
}

/* Duplicate string with max length */
char *mem_strndup(MemoryContext *ctx, const char *str, size_t n) {
    if (!str) {
        return NULL;
    }

    size_t len = strlen(str);
    if (len > n) {
        len = n;
    }

    char *copy = mem_alloc(ctx, len + 1);
    if (!copy) {
        return NULL;
    }

    memcpy(copy, str, len);
    copy[len] = '\0';
    return copy;
}

/* Free memory from context */
void mem_free(MemoryContext *ctx, void *ptr) {
    if (!ptr) {
        return;
    }

    if (ctx) {
        untrack_allocation(ctx, ptr);
    }

    free(ptr);
}

/* Allocate CreateTableStmt */
CreateTableStmt *create_table_stmt_alloc(MemoryContext *ctx) {
    (void)ctx; /* Ignore - use malloc for independent allocation */
    CreateTableStmt *stmt = calloc(1, sizeof(CreateTableStmt));
    return stmt;
}

/* Allocate ColumnDef */
ColumnDef *column_def_alloc(MemoryContext *ctx) {
    (void)ctx; /* Ignore - use malloc for independent allocation */
    ColumnDef *col = calloc(1, sizeof(ColumnDef));
    return col;
}

/* Allocate TableConstraint */
TableConstraint *table_constraint_alloc(MemoryContext *ctx) {
    (void)ctx; /* Ignore - use malloc for independent allocation */
    TableConstraint *constraint = calloc(1, sizeof(TableConstraint));
    return constraint;
}

/* Allocate ColumnConstraint */
ColumnConstraint *column_constraint_alloc(MemoryContext *ctx) {
    (void)ctx; /* Ignore - use malloc for independent allocation */
    ColumnConstraint *constraint = calloc(1, sizeof(ColumnConstraint));
    return constraint;
}

/* Allocate Expression */
Expression *expression_alloc(MemoryContext *ctx, const char *expr_text) {
    (void)ctx; /* Ignore - use malloc for independent allocation */
    Expression *expr = calloc(1, sizeof(Expression));
    if (expr && expr_text) {
        expr->expression = strdup(expr_text);
    }
    return expr;
}

/* Allocate TableElement */
TableElement *table_element_alloc(MemoryContext *ctx) {
    (void)ctx; /* Ignore - use malloc for independent allocation */
    TableElement *elem = calloc(1, sizeof(TableElement));
    return elem;
}

/* Allocate StorageParameter */
StorageParameter *storage_parameter_alloc(MemoryContext *ctx) {
    (void)ctx; /* Ignore - use malloc for independent allocation */
    StorageParameter *param = calloc(1, sizeof(StorageParameter));
    return param;
}

/* Helper: Free IndexParameters */
static void free_index_parameters(IndexParameters *params) {
    if (!params) {
        return;
    }
    if (params->include) {
        for (int i = 0; i < params->include->column_count; i++) {
            free(params->include->columns[i]);
        }
        free(params->include->columns);
        free(params->include);
    }
    free_storage_parameter_list(params->with_options);
    free(params->tablespace_name);
    free(params);
}

/* Helper: Free ExcludeElement */
static void free_exclude_element(ExcludeElement *elem) {
    if (!elem) {
        return;
    }
    free(elem->column_name);
    free_expression(elem->expression);
    free(elem->collation);
    if (elem->opclass) {
        free(elem->opclass->opclass);
        for (int i = 0; i < elem->opclass->parameter_count; i++) {
            free(elem->opclass->parameters[i].name);
            free(elem->opclass->parameters[i].value);
        }
        free(elem->opclass->parameters);
        free(elem->opclass);
    }
}

/* Helper: Free PartitionBoundSpec */
static void free_partition_bound_spec(PartitionBoundSpec *spec) {
    if (!spec) {
        return;
    }
    switch (spec->type) {
        case BOUND_TYPE_IN:
            for (int i = 0; i < spec->spec.in_bound.expr_count; i++) {
                free_expression(spec->spec.in_bound.exprs[i]);
            }
            free(spec->spec.in_bound.exprs);
            break;
        case BOUND_TYPE_RANGE:
            for (int i = 0; i < spec->spec.range_bound.from_count; i++) {
                if (spec->spec.range_bound.from_values[i].expr) {
                    free_expression(spec->spec.range_bound.from_values[i].expr);
                }
            }
            free(spec->spec.range_bound.from_values);
            for (int i = 0; i < spec->spec.range_bound.to_count; i++) {
                if (spec->spec.range_bound.to_values[i].expr) {
                    free_expression(spec->spec.range_bound.to_values[i].expr);
                }
            }
            free(spec->spec.range_bound.to_values);
            break;
        case BOUND_TYPE_HASH:
        case BOUND_TYPE_DEFAULT:
            break;
    }
    free(spec);
}

/* Helper: Free TableElement */
static void free_table_element(TableElement *elem) {
    if (!elem) {
        return;
    }

    switch (elem->type) {
        case TABLE_ELEM_COLUMN:
            /* Free column definition inline data (ColumnDef is by value) */
            free(elem->elem.column.column_name);
            free(elem->elem.column.data_type);
            free(elem->elem.column.compression_method);
            free(elem->elem.column.collation);

            /* Free column constraints linked list */
            {
                ColumnConstraint *constraint = elem->elem.column.constraints;
                while (constraint) {
                    ColumnConstraint *next = constraint->next;
                    free_column_constraint(constraint);
                    constraint = next;
                }
            }
            break;

        case TABLE_ELEM_TABLE_CONSTRAINT:
            free_table_constraint(elem->elem.table_constraint);
            break;

        case TABLE_ELEM_LIKE:
            /* LikeClause is by value */
            free(elem->elem.like.source_table);
            free(elem->elem.like.options);
            break;
    }

    free(elem);
}

/* Helper: Free TypedTableElement */
static void free_typed_table_element(TypedTableElement *elem) {
    if (!elem) {
        return;
    }

    switch (elem->type) {
        case TYPED_ELEM_COLUMN:
            free(elem->elem.column.column_name);
            {
                ColumnConstraint *constraint = elem->elem.column.constraints;
                while (constraint) {
                    ColumnConstraint *next = constraint->next;
                    free_column_constraint(constraint);
                    constraint = next;
                }
            }
            break;

        case TYPED_ELEM_TABLE_CONSTRAINT:
            free_table_constraint(elem->elem.table_constraint);
            break;
    }

    free(elem);
}

/* Free CreateTableStmt - recursively free all nested structures */
void free_create_table_stmt(CreateTableStmt *stmt) {
    if (!stmt) {
        return;
    }

    free(stmt->table_name);
    free(stmt->using_method);
    free(stmt->tablespace_name);
    free_partition_by_clause(stmt->partition_by);
    free_storage_parameter_list(stmt->with_options);

    /* Free variant-specific data */
    switch (stmt->variant) {
        case CREATE_TABLE_REGULAR:
            /* Free table elements (columns, constraints, LIKE clauses) */
            {
                TableElement *elem = stmt->table_def.regular.elements;
                while (elem) {
                    TableElement *next = elem->next;
                    free_table_element(elem);
                    elem = next;
                }
            }

            /* Free INHERITS list */
            if (stmt->table_def.regular.inherits) {
                for (int i = 0; i < stmt->table_def.regular.inherits_count; i++) {
                    free(stmt->table_def.regular.inherits[i]);
                }
                free(stmt->table_def.regular.inherits);
            }
            break;

        case CREATE_TABLE_OF_TYPE:
            /* Free type name */
            free(stmt->table_def.of_type.type_name);

            /* Free typed table elements */
            {
                TypedTableElement *elem = stmt->table_def.of_type.elements;
                while (elem) {
                    TypedTableElement *next = elem->next;
                    free_typed_table_element(elem);
                    elem = next;
                }
            }
            break;

        case CREATE_TABLE_PARTITION:
            /* Free parent table name */
            free(stmt->table_def.partition.parent_table);

            /* Free partition table elements (same as TypedTableElement) */
            {
                PartitionTableElement *elem = stmt->table_def.partition.elements;
                while (elem) {
                    PartitionTableElement *next = elem->next;
                    free_typed_table_element((TypedTableElement *)elem);
                    elem = next;
                }
            }

            /* Free partition bound spec */
            free_partition_bound_spec(stmt->table_def.partition.bound_spec);
            break;
    }

    free(stmt);
}

/* Free ColumnDef */
void free_column_def(ColumnDef *col) {
    if (!col) {
        return;
    }
    /* Rely on memory context for cleanup */
}

/* Free TableConstraint */
void free_table_constraint(TableConstraint *constraint) {
    if (!constraint) {
        return;
    }

    free(constraint->constraint_name);

    switch (constraint->type) {
        case TABLE_CONSTRAINT_CHECK:
            free_expression(constraint->constraint.check.expr);
            break;
        case TABLE_CONSTRAINT_NOT_NULL:
            free(constraint->constraint.not_null.column_name);
            break;
        case TABLE_CONSTRAINT_UNIQUE:
            for (int i = 0; i < constraint->constraint.unique.column_count; i++) {
                free(constraint->constraint.unique.columns[i]);
            }
            free(constraint->constraint.unique.columns);
            free(constraint->constraint.unique.without_overlaps_column);
            free_index_parameters(constraint->constraint.unique.index_params);
            break;
        case TABLE_CONSTRAINT_PRIMARY_KEY:
            for (int i = 0; i < constraint->constraint.primary_key.column_count; i++) {
                free(constraint->constraint.primary_key.columns[i]);
            }
            free(constraint->constraint.primary_key.columns);
            free(constraint->constraint.primary_key.without_overlaps_column);
            free_index_parameters(constraint->constraint.primary_key.index_params);
            break;
        case TABLE_CONSTRAINT_EXCLUDE:
            for (int i = 0; i < constraint->constraint.exclude.element_count; i++) {
                free_exclude_element(&constraint->constraint.exclude.elements[i]);
            }
            free(constraint->constraint.exclude.elements);
            for (int i = 0; i < constraint->constraint.exclude.element_count; i++) {
                free(constraint->constraint.exclude.operators[i]);
            }
            free(constraint->constraint.exclude.operators);
            free(constraint->constraint.exclude.index_method);
            free_index_parameters(constraint->constraint.exclude.index_params);
            free_expression(constraint->constraint.exclude.where_predicate);
            break;
        case TABLE_CONSTRAINT_FOREIGN_KEY:
            for (int i = 0; i < constraint->constraint.foreign_key.column_count; i++) {
                free(constraint->constraint.foreign_key.columns[i]);
            }
            free(constraint->constraint.foreign_key.columns);
            free(constraint->constraint.foreign_key.period_column);
            free(constraint->constraint.foreign_key.reftable);
            for (int i = 0; i < constraint->constraint.foreign_key.refcolumn_count; i++) {
                free(constraint->constraint.foreign_key.refcolumns[i]);
            }
            free(constraint->constraint.foreign_key.refcolumns);
            free(constraint->constraint.foreign_key.ref_period_column);
            for (int i = 0; i < constraint->constraint.foreign_key.on_delete_column_count; i++) {
                free(constraint->constraint.foreign_key.on_delete_columns[i]);
            }
            free(constraint->constraint.foreign_key.on_delete_columns);
            for (int i = 0; i < constraint->constraint.foreign_key.on_update_column_count; i++) {
                free(constraint->constraint.foreign_key.on_update_columns[i]);
            }
            free(constraint->constraint.foreign_key.on_update_columns);
            break;
    }

    /* Note: Do not free 'next' - caller manages the linked list */
    free(constraint);
}

/* Free ColumnConstraint */
void free_column_constraint(ColumnConstraint *constraint) {
    if (!constraint) {
        return;
    }

    free(constraint->constraint_name);

    switch (constraint->type) {
        case CONSTRAINT_NOT_NULL:
        case CONSTRAINT_NULL:
            break;
        case CONSTRAINT_CHECK:
            free_expression(constraint->constraint.check.expr);
            break;
        case CONSTRAINT_DEFAULT:
            free_expression(constraint->constraint.default_val.expr);
            break;
        case CONSTRAINT_GENERATED_ALWAYS:
            free_expression(constraint->constraint.generated_always.expr);
            break;
        case CONSTRAINT_GENERATED_IDENTITY:
            /* TODO: Free sequence_opts when implemented */
            break;
        case CONSTRAINT_UNIQUE:
            free_index_parameters(constraint->constraint.unique.index_params);
            break;
        case CONSTRAINT_PRIMARY_KEY:
            free_index_parameters(constraint->constraint.primary_key.index_params);
            break;
        case CONSTRAINT_REFERENCES:
            free(constraint->constraint.references.reftable);
            free(constraint->constraint.references.refcolumn);
            break;
    }

    /* Note: Do not free 'next' - caller manages the linked list */
    free(constraint);
}

/* Free Expression */
void free_expression(Expression *expr) {
    if (!expr) {
        return;
    }
    free(expr->expression);
    free(expr);
}

/* Free PartitionByClause */
void free_partition_by_clause(PartitionByClause *partition) {
    if (!partition) {
        return;
    }
    for (int i = 0; i < partition->element_count; i++) {
        free(partition->elements[i].column_name);
        free_expression(partition->elements[i].expression);
        free(partition->elements[i].collation);
        free(partition->elements[i].opclass);
    }
    free(partition->elements);
    free(partition);
}

/* Free StorageParameterList */
void free_storage_parameter_list(StorageParameterList *params) {
    if (!params) {
        return;
    }
    for (int i = 0; i < params->count; i++) {
        free(params->parameters[i].name);
        free(params->parameters[i].value);
    }
    free(params->parameters);
    free(params);
}

/* Clone CreateTableStmt (stub) */
CreateTableStmt *clone_create_table_stmt(const CreateTableStmt *src, MemoryContext *ctx) {
    (void)ctx;  /* Unused for now */
    if (!src) {
        return NULL;
    }
    /* TODO: Implement deep copy */
    return NULL;
}

/* Clone ColumnDef (stub) */
ColumnDef *clone_column_def(const ColumnDef *src, MemoryContext *ctx) {
    (void)ctx;  /* Unused for now */
    if (!src) {
        return NULL;
    }
    /* TODO: Implement deep copy */
    return NULL;
}

/* Get total allocated memory */
size_t memory_context_get_allocated(MemoryContext *ctx) {
    return ctx ? ctx->total_allocated : 0;
}

/* Print memory statistics */
void memory_context_stats(MemoryContext *ctx) {
    if (!ctx) {
        return;
    }

    printf("Memory Context '%s':\n", ctx->name);
    printf("  Blocks: %zu\n", ctx->block_count);
    printf("  Total allocated: %zu bytes\n", ctx->total_allocated);
}
