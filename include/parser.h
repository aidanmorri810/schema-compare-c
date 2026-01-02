#ifndef PARSER_H
#define PARSER_H

#include "pg_create_table.h"
#include "lexer.h"
#include "sc_memory.h"
#include <stdbool.h>

/* Parse error structure */
typedef struct ParseError {
    char *message;
    int line;
    int column;
    struct ParseError *next;
} ParseError;

/* Parser state */
typedef struct {
    Lexer lexer;
    Token current;
    Token previous;
    ParseError *errors;
    bool had_error;
    bool panic_mode;           /* for error recovery */
    MemoryContext *memory_ctx; /* for allocations */
} Parser;

/* Main parser API */
Parser *parser_create(const char *source);
void parser_destroy(Parser *parser);
CreateTableStmt *parser_parse_create_table(Parser *parser);
ParseError *parser_get_errors(Parser *parser);
void parser_free_errors(ParseError *errors);

/* Parse result - can hold either success or errors */
typedef struct {
    CreateTableStmt *stmt;
    ParseError *errors;
    bool success;
} ParseResult;

ParseResult *parse_ddl_file(const char *filename);
ParseResult *parse_ddl_string(const char *ddl);
void parse_result_free(ParseResult *result);

/* Token navigation functions */
bool parser_match(Parser *parser, TokenType type);
bool parser_check(Parser *parser, TokenType type);
Token parser_advance(Parser *parser);
bool parser_expect(Parser *parser, TokenType type, const char *message);
void parser_error(Parser *parser, const char *format, ...);
void parser_synchronize(Parser *parser);

/* Recursive descent parsing functions */
TableElement *parse_table_element(Parser *parser);
ColumnDef *parse_column_def(Parser *parser);
TableConstraint *parse_table_constraint(Parser *parser);
ColumnConstraint *parse_column_constraint(Parser *parser);
Expression *parse_expression(Parser *parser);
char *parse_data_type(Parser *parser);
PartitionByClause *parse_partition_by(Parser *parser);
PartitionBoundSpec *parse_partition_bound_spec(Parser *parser);
StorageParameterList *parse_with_options(Parser *parser);
LikeClause *parse_like_clause(Parser *parser);

#endif /* PARSER_H */
