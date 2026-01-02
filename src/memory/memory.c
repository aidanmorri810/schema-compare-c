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
            if (constraint->constraint.generated_identity.sequence_opts) {
                free(constraint->constraint.generated_identity.sequence_opts);
            }
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

/* Helper: Clone Expression */
static Expression *clone_expression(const Expression *src, MemoryContext *ctx) {
    if (!src) {
        return NULL;
    }
    Expression *dst = expression_alloc(ctx, src->expression);
    return dst;
}

/* Helper: Clone SequenceOptions */
static SequenceOptions *clone_sequence_options(const SequenceOptions *src, MemoryContext *ctx) {
    if (!src) {
        return NULL;
    }
    SequenceOptions *dst = mem_alloc(ctx, sizeof(SequenceOptions));
    if (!dst) {
        return NULL;
    }
    memcpy(dst, src, sizeof(SequenceOptions));
    return dst;
}

/* Helper: Clone StorageParameterList */
static StorageParameterList *clone_storage_parameter_list(const StorageParameterList *src, MemoryContext *ctx) {
    if (!src) {
        return NULL;
    }
    StorageParameterList *dst = mem_alloc(ctx, sizeof(StorageParameterList));
    if (!dst) {
        return NULL;
    }
    dst->count = src->count;
    dst->parameters = mem_alloc(ctx, sizeof(StorageParameter) * src->count);
    if (!dst->parameters) {
        return NULL;
    }
    for (int i = 0; i < src->count; i++) {
        dst->parameters[i].name = mem_strdup(ctx, src->parameters[i].name);
        dst->parameters[i].value = mem_strdup(ctx, src->parameters[i].value);
    }
    return dst;
}

/* Helper: Clone OpclassSpec */
static OpclassSpec *clone_opclass_spec(const OpclassSpec *src, MemoryContext *ctx) {
    if (!src) {
        return NULL;
    }
    OpclassSpec *dst = mem_alloc(ctx, sizeof(OpclassSpec));
    if (!dst) {
        return NULL;
    }
    dst->opclass = mem_strdup(ctx, src->opclass);
    dst->parameter_count = src->parameter_count;
    if (src->parameter_count > 0) {
        dst->parameters = mem_alloc(ctx, sizeof(StorageParameter) * src->parameter_count);
        for (int i = 0; i < src->parameter_count; i++) {
            dst->parameters[i].name = mem_strdup(ctx, src->parameters[i].name);
            dst->parameters[i].value = mem_strdup(ctx, src->parameters[i].value);
        }
    } else {
        dst->parameters = NULL;
    }
    return dst;
}

/* Helper: Clone IncludeClause */
static IncludeClause *clone_include_clause(const IncludeClause *src, MemoryContext *ctx) {
    if (!src) {
        return NULL;
    }
    IncludeClause *dst = mem_alloc(ctx, sizeof(IncludeClause));
    if (!dst) {
        return NULL;
    }
    dst->column_count = src->column_count;
    dst->columns = mem_alloc(ctx, sizeof(char *) * src->column_count);
    for (int i = 0; i < src->column_count; i++) {
        dst->columns[i] = mem_strdup(ctx, src->columns[i]);
    }
    return dst;
}

/* Helper: Clone IndexParameters */
static IndexParameters *clone_index_parameters(const IndexParameters *src, MemoryContext *ctx) {
    if (!src) {
        return NULL;
    }
    IndexParameters *dst = mem_alloc(ctx, sizeof(IndexParameters));
    if (!dst) {
        return NULL;
    }
    dst->include = clone_include_clause(src->include, ctx);
    dst->with_options = clone_storage_parameter_list(src->with_options, ctx);
    dst->tablespace_name = mem_strdup(ctx, src->tablespace_name);
    return dst;
}

/* Helper: Clone ExcludeElement */
static void clone_exclude_element(ExcludeElement *dst, const ExcludeElement *src, MemoryContext *ctx) {
    if (!dst || !src) {
        return;
    }
    dst->column_name = mem_strdup(ctx, src->column_name);
    dst->expression = clone_expression(src->expression, ctx);
    dst->collation = mem_strdup(ctx, src->collation);
    dst->opclass = clone_opclass_spec(src->opclass, ctx);
    dst->sort_order = src->sort_order;
    dst->nulls_order = src->nulls_order;
    dst->has_sort_order = src->has_sort_order;
    dst->has_nulls_order = src->has_nulls_order;
}

/* Helper: Clone ColumnConstraint */
static ColumnConstraint *clone_column_constraint(const ColumnConstraint *src, MemoryContext *ctx) {
    if (!src) {
        return NULL;
    }
    ColumnConstraint *dst = column_constraint_alloc(ctx);
    if (!dst) {
        return NULL;
    }

    dst->constraint_name = mem_strdup(ctx, src->constraint_name);
    dst->type = src->type;
    dst->deferrable = src->deferrable;
    dst->not_deferrable = src->not_deferrable;
    dst->initially_deferred = src->initially_deferred;
    dst->initially_immediate = src->initially_immediate;
    dst->enforced = src->enforced;
    dst->not_enforced = src->not_enforced;
    dst->has_deferrable = src->has_deferrable;
    dst->has_initially = src->has_initially;
    dst->has_enforced = src->has_enforced;

    switch (src->type) {
        case CONSTRAINT_NOT_NULL:
            dst->constraint.not_null = src->constraint.not_null;
            break;
        case CONSTRAINT_NULL:
            break;
        case CONSTRAINT_CHECK:
            dst->constraint.check.expr = clone_expression(src->constraint.check.expr, ctx);
            dst->constraint.check.no_inherit = src->constraint.check.no_inherit;
            break;
        case CONSTRAINT_DEFAULT:
            dst->constraint.default_val.expr = clone_expression(src->constraint.default_val.expr, ctx);
            break;
        case CONSTRAINT_GENERATED_ALWAYS:
            dst->constraint.generated_always.expr = clone_expression(src->constraint.generated_always.expr, ctx);
            dst->constraint.generated_always.storage = src->constraint.generated_always.storage;
            dst->constraint.generated_always.has_storage = src->constraint.generated_always.has_storage;
            break;
        case CONSTRAINT_GENERATED_IDENTITY:
            dst->constraint.generated_identity.type = src->constraint.generated_identity.type;
            dst->constraint.generated_identity.sequence_opts =
                clone_sequence_options(src->constraint.generated_identity.sequence_opts, ctx);
            break;
        case CONSTRAINT_UNIQUE:
            dst->constraint.unique.nulls_distinct = src->constraint.unique.nulls_distinct;
            dst->constraint.unique.has_nulls_distinct = src->constraint.unique.has_nulls_distinct;
            dst->constraint.unique.index_params = clone_index_parameters(src->constraint.unique.index_params, ctx);
            break;
        case CONSTRAINT_PRIMARY_KEY:
            dst->constraint.primary_key.index_params =
                clone_index_parameters(src->constraint.primary_key.index_params, ctx);
            break;
        case CONSTRAINT_REFERENCES:
            dst->constraint.references.reftable = mem_strdup(ctx, src->constraint.references.reftable);
            dst->constraint.references.refcolumn = mem_strdup(ctx, src->constraint.references.refcolumn);
            dst->constraint.references.match_type = src->constraint.references.match_type;
            dst->constraint.references.has_match_type = src->constraint.references.has_match_type;
            dst->constraint.references.on_delete = src->constraint.references.on_delete;
            dst->constraint.references.has_on_delete = src->constraint.references.has_on_delete;
            dst->constraint.references.on_update = src->constraint.references.on_update;
            dst->constraint.references.has_on_update = src->constraint.references.has_on_update;
            break;
    }

    dst->next = NULL;
    return dst;
}

/* Helper: Clone TableConstraint */
static TableConstraint *clone_table_constraint(const TableConstraint *src, MemoryContext *ctx) {
    if (!src) {
        return NULL;
    }
    TableConstraint *dst = table_constraint_alloc(ctx);
    if (!dst) {
        return NULL;
    }

    dst->constraint_name = mem_strdup(ctx, src->constraint_name);
    dst->type = src->type;
    dst->deferrable = src->deferrable;
    dst->not_deferrable = src->not_deferrable;
    dst->initially_deferred = src->initially_deferred;
    dst->initially_immediate = src->initially_immediate;
    dst->enforced = src->enforced;
    dst->not_enforced = src->not_enforced;
    dst->has_deferrable = src->has_deferrable;
    dst->has_initially = src->has_initially;
    dst->has_enforced = src->has_enforced;

    switch (src->type) {
        case TABLE_CONSTRAINT_CHECK:
            dst->constraint.check.expr = clone_expression(src->constraint.check.expr, ctx);
            dst->constraint.check.no_inherit = src->constraint.check.no_inherit;
            break;
        case TABLE_CONSTRAINT_NOT_NULL:
            dst->constraint.not_null.column_name = mem_strdup(ctx, src->constraint.not_null.column_name);
            dst->constraint.not_null.no_inherit = src->constraint.not_null.no_inherit;
            break;
        case TABLE_CONSTRAINT_UNIQUE:
            dst->constraint.unique.column_count = src->constraint.unique.column_count;
            dst->constraint.unique.columns = mem_alloc(ctx, sizeof(char *) * src->constraint.unique.column_count);
            for (int i = 0; i < src->constraint.unique.column_count; i++) {
                dst->constraint.unique.columns[i] = mem_strdup(ctx, src->constraint.unique.columns[i]);
            }
            dst->constraint.unique.without_overlaps_column = mem_strdup(ctx, src->constraint.unique.without_overlaps_column);
            dst->constraint.unique.nulls_distinct = src->constraint.unique.nulls_distinct;
            dst->constraint.unique.has_nulls_distinct = src->constraint.unique.has_nulls_distinct;
            dst->constraint.unique.index_params = clone_index_parameters(src->constraint.unique.index_params, ctx);
            break;
        case TABLE_CONSTRAINT_PRIMARY_KEY:
            dst->constraint.primary_key.column_count = src->constraint.primary_key.column_count;
            dst->constraint.primary_key.columns = mem_alloc(ctx, sizeof(char *) * src->constraint.primary_key.column_count);
            for (int i = 0; i < src->constraint.primary_key.column_count; i++) {
                dst->constraint.primary_key.columns[i] = mem_strdup(ctx, src->constraint.primary_key.columns[i]);
            }
            dst->constraint.primary_key.without_overlaps_column =
                mem_strdup(ctx, src->constraint.primary_key.without_overlaps_column);
            dst->constraint.primary_key.index_params =
                clone_index_parameters(src->constraint.primary_key.index_params, ctx);
            break;
        case TABLE_CONSTRAINT_EXCLUDE:
            dst->constraint.exclude.index_method = mem_strdup(ctx, src->constraint.exclude.index_method);
            dst->constraint.exclude.element_count = src->constraint.exclude.element_count;
            dst->constraint.exclude.elements = mem_alloc(ctx, sizeof(ExcludeElement) * src->constraint.exclude.element_count);
            for (int i = 0; i < src->constraint.exclude.element_count; i++) {
                clone_exclude_element(&dst->constraint.exclude.elements[i], &src->constraint.exclude.elements[i], ctx);
            }
            dst->constraint.exclude.operators = mem_alloc(ctx, sizeof(char *) * src->constraint.exclude.element_count);
            for (int i = 0; i < src->constraint.exclude.element_count; i++) {
                dst->constraint.exclude.operators[i] = mem_strdup(ctx, src->constraint.exclude.operators[i]);
            }
            dst->constraint.exclude.index_params = clone_index_parameters(src->constraint.exclude.index_params, ctx);
            dst->constraint.exclude.where_predicate = clone_expression(src->constraint.exclude.where_predicate, ctx);
            break;
        case TABLE_CONSTRAINT_FOREIGN_KEY:
            dst->constraint.foreign_key.column_count = src->constraint.foreign_key.column_count;
            dst->constraint.foreign_key.columns = mem_alloc(ctx, sizeof(char *) * src->constraint.foreign_key.column_count);
            for (int i = 0; i < src->constraint.foreign_key.column_count; i++) {
                dst->constraint.foreign_key.columns[i] = mem_strdup(ctx, src->constraint.foreign_key.columns[i]);
            }
            dst->constraint.foreign_key.period_column = mem_strdup(ctx, src->constraint.foreign_key.period_column);
            dst->constraint.foreign_key.reftable = mem_strdup(ctx, src->constraint.foreign_key.reftable);
            dst->constraint.foreign_key.refcolumn_count = src->constraint.foreign_key.refcolumn_count;
            dst->constraint.foreign_key.refcolumns = mem_alloc(ctx, sizeof(char *) * src->constraint.foreign_key.refcolumn_count);
            for (int i = 0; i < src->constraint.foreign_key.refcolumn_count; i++) {
                dst->constraint.foreign_key.refcolumns[i] = mem_strdup(ctx, src->constraint.foreign_key.refcolumns[i]);
            }
            dst->constraint.foreign_key.ref_period_column = mem_strdup(ctx, src->constraint.foreign_key.ref_period_column);
            dst->constraint.foreign_key.match_type = src->constraint.foreign_key.match_type;
            dst->constraint.foreign_key.has_match_type = src->constraint.foreign_key.has_match_type;
            dst->constraint.foreign_key.on_delete = src->constraint.foreign_key.on_delete;
            dst->constraint.foreign_key.has_on_delete = src->constraint.foreign_key.has_on_delete;
            dst->constraint.foreign_key.on_update = src->constraint.foreign_key.on_update;
            dst->constraint.foreign_key.has_on_update = src->constraint.foreign_key.has_on_update;
            dst->constraint.foreign_key.on_delete_column_count = src->constraint.foreign_key.on_delete_column_count;
            if (src->constraint.foreign_key.on_delete_column_count > 0) {
                dst->constraint.foreign_key.on_delete_columns =
                    mem_alloc(ctx, sizeof(char *) * src->constraint.foreign_key.on_delete_column_count);
                for (int i = 0; i < src->constraint.foreign_key.on_delete_column_count; i++) {
                    dst->constraint.foreign_key.on_delete_columns[i] =
                        mem_strdup(ctx, src->constraint.foreign_key.on_delete_columns[i]);
                }
            } else {
                dst->constraint.foreign_key.on_delete_columns = NULL;
            }
            dst->constraint.foreign_key.on_update_column_count = src->constraint.foreign_key.on_update_column_count;
            if (src->constraint.foreign_key.on_update_column_count > 0) {
                dst->constraint.foreign_key.on_update_columns =
                    mem_alloc(ctx, sizeof(char *) * src->constraint.foreign_key.on_update_column_count);
                for (int i = 0; i < src->constraint.foreign_key.on_update_column_count; i++) {
                    dst->constraint.foreign_key.on_update_columns[i] =
                        mem_strdup(ctx, src->constraint.foreign_key.on_update_columns[i]);
                }
            } else {
                dst->constraint.foreign_key.on_update_columns = NULL;
            }
            break;
    }

    dst->next = NULL;
    return dst;
}

/* Helper: Clone PartitionByClause */
static PartitionByClause *clone_partition_by_clause(const PartitionByClause *src, MemoryContext *ctx) {
    if (!src) {
        return NULL;
    }
    PartitionByClause *dst = mem_alloc(ctx, sizeof(PartitionByClause));
    if (!dst) {
        return NULL;
    }
    dst->type = src->type;
    dst->element_count = src->element_count;
    dst->elements = mem_alloc(ctx, sizeof(PartitionElement) * src->element_count);
    for (int i = 0; i < src->element_count; i++) {
        dst->elements[i].column_name = mem_strdup(ctx, src->elements[i].column_name);
        dst->elements[i].expression = clone_expression(src->elements[i].expression, ctx);
        dst->elements[i].collation = mem_strdup(ctx, src->elements[i].collation);
        dst->elements[i].opclass = mem_strdup(ctx, src->elements[i].opclass);
    }
    return dst;
}

/* Helper: Clone PartitionBoundSpec */
static PartitionBoundSpec *clone_partition_bound_spec(const PartitionBoundSpec *src, MemoryContext *ctx) {
    if (!src) {
        return NULL;
    }
    PartitionBoundSpec *dst = mem_alloc(ctx, sizeof(PartitionBoundSpec));
    if (!dst) {
        return NULL;
    }
    dst->type = src->type;

    switch (src->type) {
        case BOUND_TYPE_IN:
            dst->spec.in_bound.expr_count = src->spec.in_bound.expr_count;
            dst->spec.in_bound.exprs = mem_alloc(ctx, sizeof(Expression *) * src->spec.in_bound.expr_count);
            for (int i = 0; i < src->spec.in_bound.expr_count; i++) {
                dst->spec.in_bound.exprs[i] = clone_expression(src->spec.in_bound.exprs[i], ctx);
            }
            break;
        case BOUND_TYPE_RANGE:
            dst->spec.range_bound.from_count = src->spec.range_bound.from_count;
            dst->spec.range_bound.from_values = mem_alloc(ctx, sizeof(BoundValue) * src->spec.range_bound.from_count);
            for (int i = 0; i < src->spec.range_bound.from_count; i++) {
                dst->spec.range_bound.from_values[i].is_minvalue = src->spec.range_bound.from_values[i].is_minvalue;
                dst->spec.range_bound.from_values[i].is_maxvalue = src->spec.range_bound.from_values[i].is_maxvalue;
                dst->spec.range_bound.from_values[i].expr =
                    clone_expression(src->spec.range_bound.from_values[i].expr, ctx);
            }
            dst->spec.range_bound.to_count = src->spec.range_bound.to_count;
            dst->spec.range_bound.to_values = mem_alloc(ctx, sizeof(BoundValue) * src->spec.range_bound.to_count);
            for (int i = 0; i < src->spec.range_bound.to_count; i++) {
                dst->spec.range_bound.to_values[i].is_minvalue = src->spec.range_bound.to_values[i].is_minvalue;
                dst->spec.range_bound.to_values[i].is_maxvalue = src->spec.range_bound.to_values[i].is_maxvalue;
                dst->spec.range_bound.to_values[i].expr =
                    clone_expression(src->spec.range_bound.to_values[i].expr, ctx);
            }
            break;
        case BOUND_TYPE_HASH:
            dst->spec.hash_bound.modulus = src->spec.hash_bound.modulus;
            dst->spec.hash_bound.remainder = src->spec.hash_bound.remainder;
            break;
        case BOUND_TYPE_DEFAULT:
            break;
    }

    return dst;
}

/* Helper: Clone TableElement */
static TableElement *clone_table_element(const TableElement *src, MemoryContext *ctx) {
    if (!src) {
        return NULL;
    }
    TableElement *dst = table_element_alloc(ctx);
    if (!dst) {
        return NULL;
    }

    dst->type = src->type;

    switch (src->type) {
        case TABLE_ELEM_COLUMN:
            dst->elem.column.column_name = mem_strdup(ctx, src->elem.column.column_name);
            dst->elem.column.data_type = mem_strdup(ctx, src->elem.column.data_type);
            dst->elem.column.storage_type = src->elem.column.storage_type;
            dst->elem.column.has_storage = src->elem.column.has_storage;
            dst->elem.column.compression_method = mem_strdup(ctx, src->elem.column.compression_method);
            dst->elem.column.collation = mem_strdup(ctx, src->elem.column.collation);

            /* Clone constraints linked list */
            dst->elem.column.constraints = NULL;
            ColumnConstraint **dst_constraint_ptr = &dst->elem.column.constraints;
            const ColumnConstraint *src_constraint = src->elem.column.constraints;
            while (src_constraint) {
                *dst_constraint_ptr = clone_column_constraint(src_constraint, ctx);
                dst_constraint_ptr = &((*dst_constraint_ptr)->next);
                src_constraint = src_constraint->next;
            }
            break;

        case TABLE_ELEM_TABLE_CONSTRAINT:
            dst->elem.table_constraint = clone_table_constraint(src->elem.table_constraint, ctx);
            break;

        case TABLE_ELEM_LIKE:
            dst->elem.like.source_table = mem_strdup(ctx, src->elem.like.source_table);
            dst->elem.like.option_count = src->elem.like.option_count;
            if (src->elem.like.option_count > 0) {
                dst->elem.like.options = mem_alloc(ctx, sizeof(LikeOption) * src->elem.like.option_count);
                memcpy(dst->elem.like.options, src->elem.like.options, sizeof(LikeOption) * src->elem.like.option_count);
            } else {
                dst->elem.like.options = NULL;
            }
            break;
    }

    dst->next = NULL;
    return dst;
}

/* Helper: Clone TypedTableElement */
static TypedTableElement *clone_typed_table_element(const TypedTableElement *src, MemoryContext *ctx) {
    if (!src) {
        return NULL;
    }
    TypedTableElement *dst = mem_alloc(ctx, sizeof(TypedTableElement));
    if (!dst) {
        return NULL;
    }

    dst->type = src->type;

    switch (src->type) {
        case TYPED_ELEM_COLUMN:
            dst->elem.column.column_name = mem_strdup(ctx, src->elem.column.column_name);
            dst->elem.column.with_options = src->elem.column.with_options;

            /* Clone constraints linked list */
            dst->elem.column.constraints = NULL;
            ColumnConstraint **dst_constraint_ptr = &dst->elem.column.constraints;
            const ColumnConstraint *src_constraint = src->elem.column.constraints;
            while (src_constraint) {
                *dst_constraint_ptr = clone_column_constraint(src_constraint, ctx);
                dst_constraint_ptr = &((*dst_constraint_ptr)->next);
                src_constraint = src_constraint->next;
            }
            break;

        case TYPED_ELEM_TABLE_CONSTRAINT:
            dst->elem.table_constraint = clone_table_constraint(src->elem.table_constraint, ctx);
            break;
    }

    dst->next = NULL;
    return dst;
}

/* Clone CreateTableStmt - deep copy implementation */
CreateTableStmt *clone_create_table_stmt(const CreateTableStmt *src, MemoryContext *ctx) {
    if (!src) {
        return NULL;
    }

    CreateTableStmt *dst = create_table_stmt_alloc(ctx);
    if (!dst) {
        return NULL;
    }

    /* Copy scalar fields */
    dst->temp_scope = src->temp_scope;
    dst->table_type = src->table_type;
    dst->if_not_exists = src->if_not_exists;
    dst->table_name = mem_strdup(ctx, src->table_name);
    dst->variant = src->variant;
    dst->without_oids = src->without_oids;
    dst->on_commit = src->on_commit;
    dst->has_on_commit = src->has_on_commit;

    /* Clone common fields */
    dst->partition_by = clone_partition_by_clause(src->partition_by, ctx);
    dst->using_method = mem_strdup(ctx, src->using_method);
    dst->with_options = clone_storage_parameter_list(src->with_options, ctx);
    dst->tablespace_name = mem_strdup(ctx, src->tablespace_name);

    /* Clone variant-specific data */
    switch (src->variant) {
        case CREATE_TABLE_REGULAR:
            /* Clone table elements linked list */
            dst->table_def.regular.elements = NULL;
            TableElement **dst_elem_ptr = &dst->table_def.regular.elements;
            const TableElement *src_elem = src->table_def.regular.elements;
            while (src_elem) {
                *dst_elem_ptr = clone_table_element(src_elem, ctx);
                dst_elem_ptr = &((*dst_elem_ptr)->next);
                src_elem = src_elem->next;
            }

            /* Clone INHERITS list */
            dst->table_def.regular.inherits_count = src->table_def.regular.inherits_count;
            if (src->table_def.regular.inherits_count > 0) {
                dst->table_def.regular.inherits =
                    mem_alloc(ctx, sizeof(char *) * src->table_def.regular.inherits_count);
                for (int i = 0; i < src->table_def.regular.inherits_count; i++) {
                    dst->table_def.regular.inherits[i] = mem_strdup(ctx, src->table_def.regular.inherits[i]);
                }
            } else {
                dst->table_def.regular.inherits = NULL;
            }
            break;

        case CREATE_TABLE_OF_TYPE:
            dst->table_def.of_type.type_name = mem_strdup(ctx, src->table_def.of_type.type_name);

            /* Clone typed table elements linked list */
            dst->table_def.of_type.elements = NULL;
            TypedTableElement **dst_typed_elem_ptr = &dst->table_def.of_type.elements;
            const TypedTableElement *src_typed_elem = src->table_def.of_type.elements;
            while (src_typed_elem) {
                *dst_typed_elem_ptr = clone_typed_table_element(src_typed_elem, ctx);
                dst_typed_elem_ptr = &((*dst_typed_elem_ptr)->next);
                src_typed_elem = src_typed_elem->next;
            }
            break;

        case CREATE_TABLE_PARTITION:
            dst->table_def.partition.parent_table = mem_strdup(ctx, src->table_def.partition.parent_table);
            dst->table_def.partition.is_default = src->table_def.partition.is_default;

            /* Clone partition table elements linked list (same as TypedTableElement) */
            dst->table_def.partition.elements = NULL;
            PartitionTableElement **dst_part_elem_ptr = &dst->table_def.partition.elements;
            const PartitionTableElement *src_part_elem = src->table_def.partition.elements;
            while (src_part_elem) {
                *dst_part_elem_ptr = (PartitionTableElement *)clone_typed_table_element(
                    (const TypedTableElement *)src_part_elem, ctx);
                dst_part_elem_ptr = &((*dst_part_elem_ptr)->next);
                src_part_elem = src_part_elem->next;
            }

            /* Clone partition bound spec */
            dst->table_def.partition.bound_spec = clone_partition_bound_spec(src->table_def.partition.bound_spec, ctx);
            break;
    }

    return dst;
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
