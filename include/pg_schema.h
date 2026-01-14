#pragma once

#include "pg_create_table.h"

typedef enum {
    STMT_TABLE,
    STMT_TYPE,
    STMT_INDEX,
    STMT_TRIGGER,
    STMT_FUNCTION,
    STMT_PROCEDURE
} StatementType;

/* Forward declarations for statement types */
typedef struct CreateTypeStmt CreateTypeStmt;
typedef struct CreateIndexStmt CreateIndexStmt;
typedef struct CreateTriggerStmt CreateTriggerStmt;
typedef struct CreateFunctionStmt CreateFunctionStmt;
typedef struct CreateProcedureStmt CreateProcedureStmt;

typedef struct SchemaStatement {
    StatementType type;
    union {
        CreateTableStmt *table;
        CreateTypeStmt *type;
        CreateIndexStmt *index;
        CreateTriggerStmt *trigger;
        CreateFunctionStmt *function;
        CreateProcedureStmt *procedure;
    } stmt;
    struct SchemaStatement *next;
} SchemaStatement;

/* Schema container for all database objects */
typedef struct Schema {
    CreateTypeStmt **types;
    int type_count;
    CreateTableStmt **tables;
    int table_count;
    CreateFunctionStmt **functions;
    int function_count;
    CreateProcedureStmt **procedures;
    int procedure_count;
    /* Note: Indexes and triggers are stored in table->indexes[] and table->triggers[] */
} Schema;