#ifndef PARSER_H
#define PARSER_H

#include "pg_schema.h"
#include "pg_create_table.h"
#include "pg_create_type.h"
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

typedef struct {
    CreateTableStmt *stmt;
    ParseError *errors;
    bool success;
} ParseResult;

/*                 */
/* Main parser API */
/*                 */
Parser *parser_create(const char *source);
void parser_destroy(Parser *parser);
ParseError *parser_get_errors(Parser *parser);
void parser_free_errors(ParseError *errors);
void parse_result_free(ParseResult *result);

/* Token navigation functions */
bool parser_match(Parser *parser, TokenType type);
bool parser_check(Parser *parser, TokenType type);
Token parser_advance(Parser *parser);
bool parser_expect(Parser *parser, TokenType type, const char *message);
void parser_error(Parser *parser, const char *format, ...);
void parser_synchronize(Parser *parser);

/* Recursive descent parsing functions */
Expression *parse_expression(Parser *parser);
StorageParameterList *parse_with_options(Parser *parser);

/*                       */
/* SQL Statement Handlers */
/*                       */

/* parse_schema.c */
Schema *parse_all_statements(Parser *parser);
void parser_parse_statement(Parser *parser, Schema *schema);

/* parse_table.c */
CreateTableStmt *parser_parse_create_table(Parser *parser);
TableElement *parse_table_element(Parser *parser);
TableConstraint *parse_table_constraint(Parser *parser);
TableElement *parse_table_element_list(Parser *parser);
LikeClause *parse_like_clause(Parser *parser);

/* parse_partition.c */
PartitionByClause *parse_partition_by(Parser *parser);
PartitionBoundSpec *parse_partition_bound_spec(Parser *parser);

/* parse_constrain.c */
ColumnConstraint *parse_column_constraint(Parser *parser);
SequenceOptions *parse_sequence_options(Parser *parser);
IndexParameters *parse_index_parameters(Parser *parser);
bool parse_constraint_attributes(Parser *parser, ColumnConstraint *constraint);

/* parse_column.c */
ColumnDef *parse_column_def(Parser *parser);
char *parse_data_type(Parser *parser);

/* parse_type.c */
CreateTypeStmt *parser_parse_create_type(Parser *parser);


#endif /* PARSER_H */
