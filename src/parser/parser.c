#include "parser.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/* Forward declarations for parse_table.c functions */
TableElement *parse_table_element_list(Parser *parser);

/* Create parser */
Parser *parser_create(const char *source) {
    Parser *parser = malloc(sizeof(Parser));
    if (!parser) {
        return NULL;
    }

    lexer_init(&parser->lexer, source);
    parser->memory_ctx = memory_context_create("Parser");
    parser->errors = NULL;
    parser->had_error = false;
    parser->panic_mode = false;

    /* Initialize with first token */
    parser->previous.type = TOKEN_EOF;
    parser->previous.lexeme = NULL;
    parser->current = lexer_next_token(&parser->lexer);

    return parser;
}

/* Destroy parser */
void parser_destroy(Parser *parser) {
    if (!parser) {
        return;
    }

    lexer_cleanup(&parser->lexer);
    lexer_free_token(&parser->current);
    lexer_free_token(&parser->previous);
    parser_free_errors(parser->errors);

    if (parser->memory_ctx) {
        memory_context_destroy(parser->memory_ctx);
    }

    free(parser);
}

/* Add error to list */
void parser_error(Parser *parser, const char *format, ...) {
    if (parser->panic_mode) {
        return;
    }

    parser->panic_mode = true;
    parser->had_error = true;

    /* Format error message */
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    /* Create error node */
    ParseError *error = malloc(sizeof(ParseError));
    if (!error) {
        return;
    }

    error->message = strdup(buffer);
    error->line = parser->current.line;
    error->column = parser->current.column;
    error->next = parser->errors;
    parser->errors = error;

    /* Log error */
    log_error("Parse error at line %d, column %d: %s",
              error->line, error->column, error->message);
}

/* Check if current token matches type */
bool parser_check(Parser *parser, TokenType type) {
    return parser->current.type == type;
}

/* Match and consume token if it matches */
bool parser_match(Parser *parser, TokenType type) {
    if (!parser_check(parser, type)) {
        return false;
    }
    parser_advance(parser);
    return true;
}

/* Advance to next token */
Token parser_advance(Parser *parser) {
    lexer_free_token(&parser->previous);
    parser->previous = parser->current;

    while (true) {
        parser->current = lexer_next_token(&parser->lexer);
        if (parser->current.type != TOKEN_ERROR) {
            break;
        }

        parser_error(parser, "Lexer error: %s", parser->current.lexeme);
    }

    return parser->previous;
}

/* Expect a specific token type */
bool parser_expect(Parser *parser, TokenType type, const char *message) {
    if (parser_check(parser, type)) {
        parser_advance(parser);
        return true;
    }

    parser_error(parser, "%s", message);
    return false;
}

/* Synchronize after error */
void parser_synchronize(Parser *parser) {
    parser->panic_mode = false;

    while (parser->current.type != TOKEN_EOF) {
        /* Stop at statement boundaries */
        if (parser->previous.type == TOKEN_SEMICOLON) {
            return;
        }

        switch (parser->current.type) {
            case TOKEN_CREATE:
            case TOKEN_ALTER:
            case TOKEN_DROP:
                return;
            default:
                ; /* Continue */
        }

        parser_advance(parser);
    }
}

/* Get errors */
ParseError *parser_get_errors(Parser *parser) {
    return parser ? parser->errors : NULL;
}

/* Free error list */
void parser_free_errors(ParseError *errors) {
    while (errors) {
        ParseError *next = errors->next;
        free(errors->message);
        free(errors);
        errors = next;
    }
}

/* Parse CREATE TABLE statement */
CreateTableStmt *parser_parse_create_table(Parser *parser) {
    if (!parser_expect(parser, TOKEN_CREATE, "Expected CREATE")) {
        return NULL;
    }

    /* Handle table modifiers */
    TempScope temp_scope = TEMP_SCOPE_NONE;
    TableType table_type = TABLE_TYPE_NORMAL;

    /* Check for GLOBAL/LOCAL */
    if (parser_match(parser, TOKEN_GLOBAL)) {
        temp_scope = TEMP_SCOPE_GLOBAL;
    } else if (parser_match(parser, TOKEN_LOCAL)) {
        temp_scope = TEMP_SCOPE_LOCAL;
    }

    /* Check for TEMPORARY/TEMP/UNLOGGED */
    if (parser_match(parser, TOKEN_TEMPORARY) || parser_match(parser, TOKEN_TEMP)) {
        table_type = TABLE_TYPE_TEMPORARY;
    } else if (parser_match(parser, TOKEN_UNLOGGED)) {
        table_type = TABLE_TYPE_UNLOGGED;
    }

    if (!parser_expect(parser, TOKEN_TABLE, "Expected TABLE")) {
        return NULL;
    }

    /* Check for IF NOT EXISTS */
    bool if_not_exists = false;
    if (parser_match(parser, TOKEN_IF)) {
        if (!parser_expect(parser, TOKEN_NOT, "Expected NOT after IF")) {
            return NULL;
        }
        if (!parser_expect(parser, TOKEN_EXISTS, "Expected EXISTS after IF NOT")) {
            return NULL;
        }
        if_not_exists = true;
    }

    /* Parse table name */
    if (!parser_check(parser, TOKEN_IDENTIFIER)) {
        parser_error(parser, "Expected table name");
        return NULL;
    }

    char *table_name = strdup(parser->current.lexeme);
    parser_advance(parser);

    /* Create statement */
    CreateTableStmt *stmt = create_table_stmt_alloc(parser->memory_ctx);
    if (!stmt) {
        return NULL;
    }

    stmt->temp_scope = temp_scope;
    stmt->table_type = table_type;
    stmt->if_not_exists = if_not_exists;
    stmt->table_name = table_name;

    /* Determine table variant and parse accordingly */
    if (parser_match(parser, TOKEN_OF)) {
        /* OF type_name table */
        stmt->variant = CREATE_TABLE_OF_TYPE;
        /* TODO: Parse OF TYPE variant */
        parser_error(parser, "OF TYPE tables not yet implemented");
        return NULL;
    } else if (parser_match(parser, TOKEN_PARTITION)) {
        /* PARTITION OF parent_table */
        if (!parser_expect(parser, TOKEN_OF, "Expected OF after PARTITION")) {
            return NULL;
        }
        stmt->variant = CREATE_TABLE_PARTITION;
        /* TODO: Parse PARTITION variant */
        parser_error(parser, "PARTITION tables not yet implemented");
        return NULL;
    } else {
        /* Regular table */
        stmt->variant = CREATE_TABLE_REGULAR;

        /* Parse table elements */
        TableElement *elements = parse_table_element_list(parser);
        stmt->table_def.regular.elements = elements;
        stmt->table_def.regular.inherits = NULL;
        stmt->table_def.regular.inherits_count = 0;

        /* TODO: Parse INHERITS, PARTITION BY, USING, WITH, TABLESPACE, ON COMMIT */
    }

    /* Consume optional semicolon to terminate the statement */
    parser_match(parser, TOKEN_SEMICOLON);

    return stmt;
}

/* Parse DDL from file */
ParseResult *parse_ddl_file(const char *filename) {
    char *source = read_file_to_string(filename);
    if (!source) {
        ParseResult *result = malloc(sizeof(ParseResult));
        if (result) {
            result->stmt = NULL;
            result->success = false;
            ParseError *error = malloc(sizeof(ParseError));
            if (error) {
                char msg[256];
                snprintf(msg, sizeof(msg), "Could not read file: %s", filename);
                error->message = strdup(msg);
                error->line = 0;
                error->column = 0;
                error->next = NULL;
                result->errors = error;
            } else {
                result->errors = NULL;
            }
        }
        return result;
    }

    ParseResult *result = parse_ddl_string(source);
    free(source);
    return result;
}

/* Parse DDL from string */
ParseResult *parse_ddl_string(const char *ddl) {
    Parser *parser = parser_create(ddl);
    if (!parser) {
        return NULL;
    }

    ParseResult *result = malloc(sizeof(ParseResult));
    if (!result) {
        parser_destroy(parser);
        return NULL;
    }

    result->stmt = parser_parse_create_table(parser);
    result->success = !parser->had_error && result->stmt != NULL;
    result->errors = parser->errors;
    parser->errors = NULL; /* Transfer ownership */

    parser_destroy(parser);
    return result;
}

/* Free parse result */
void parse_result_free(ParseResult *result) {
    if (!result) {
        return;
    }

    if (result->stmt) {
        free_create_table_stmt(result->stmt);
    }

    parser_free_errors(result->errors);
    free(result);
}
