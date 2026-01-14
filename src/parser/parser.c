#include "parser.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/* Forward declarations for parse_table.c functions */

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
