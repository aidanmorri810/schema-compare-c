#include "lexer.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

/* Keyword mapping structure */
typedef struct {
    const char *keyword;
    TokenType type;
} KeywordMap;

/* Keyword lookup table (must be sorted alphabetically for binary search) */
static const KeywordMap keywords[] = {
    {"action", TOKEN_ACTION},
    {"all", TOKEN_ALL},
    {"alter", TOKEN_ALTER},
    {"always", TOKEN_ALWAYS},
    {"as", TOKEN_AS},
    {"asc", TOKEN_ASC},
    {"by", TOKEN_BY},
    {"cache", TOKEN_CACHE},
    {"cascade", TOKEN_CASCADE},
    {"check", TOKEN_CHECK},
    {"collate", TOKEN_COLLATE},
    {"comments", TOKEN_COMMENTS},
    {"commit", TOKEN_COMMIT},
    {"compression", TOKEN_COMPRESSION},
    {"constraint", TOKEN_CONSTRAINT},
    {"constraints", TOKEN_CONSTRAINTS},
    {"create", TOKEN_CREATE},
    {"cycle", TOKEN_CYCLE},
    {"default", TOKEN_DEFAULT},
    {"defaults", TOKEN_DEFAULTS},
    {"deferrable", TOKEN_DEFERRABLE},
    {"deferred", TOKEN_DEFERRED},
    {"delete", TOKEN_DELETE},
    {"desc", TOKEN_DESC},
    {"distinct", TOKEN_DISTINCT},
    {"drop", TOKEN_DROP},
    {"enforced", TOKEN_ENFORCED},
    {"exclude", TOKEN_EXCLUDE},
    {"excluding", TOKEN_EXCLUDING},
    {"exists", TOKEN_EXISTS},
    {"extended", TOKEN_EXTENDED},
    {"external", TOKEN_EXTERNAL},
    {"first", TOKEN_FIRST},
    {"for", TOKEN_FOR},
    {"foreign", TOKEN_FOREIGN},
    {"from", TOKEN_FROM},
    {"full", TOKEN_FULL},
    {"generated", TOKEN_GENERATED},
    {"global", TOKEN_GLOBAL},
    {"hash", TOKEN_HASH},
    {"identity", TOKEN_IDENTITY},
    {"if", TOKEN_IF},
    {"in", TOKEN_IN},
    {"include", TOKEN_INCLUDE},
    {"including", TOKEN_INCLUDING},
    {"increment", TOKEN_INCREMENT},
    {"indexes", TOKEN_INDEXES},
    {"inherit", TOKEN_INHERIT},
    {"inherits", TOKEN_INHERITS},
    {"initially", TOKEN_INITIALLY},
    {"immediate", TOKEN_IMMEDIATE},
    {"key", TOKEN_KEY},
    {"last", TOKEN_LAST},
    {"like", TOKEN_LIKE},
    {"list", TOKEN_LIST},
    {"local", TOKEN_LOCAL},
    {"main", TOKEN_MAIN},
    {"match", TOKEN_MATCH},
    {"maxvalue", TOKEN_MAXVALUE},
    {"minvalue", TOKEN_MINVALUE},
    {"modulus", TOKEN_MODULUS},
    {"no", TOKEN_NO},
    {"not", TOKEN_NOT},
    {"null", TOKEN_NULL},
    {"nulls", TOKEN_NULLS},
    {"of", TOKEN_OF},
    {"oids", TOKEN_OIDS},
    {"on", TOKEN_ON},
    {"overlaps", TOKEN_OVERLAPS},
    {"owned", TOKEN_OWNED},
    {"partial", TOKEN_PARTIAL},
    {"partition", TOKEN_PARTITION},
    {"period", TOKEN_PERIOD},
    {"plain", TOKEN_PLAIN},
    {"preserve", TOKEN_PRESERVE},
    {"primary", TOKEN_PRIMARY},
    {"range", TOKEN_RANGE},
    {"references", TOKEN_REFERENCES},
    {"remainder", TOKEN_REMAINDER},
    {"restrict", TOKEN_RESTRICT},
    {"rows", TOKEN_ROWS},
    {"set", TOKEN_SET},
    {"simple", TOKEN_SIMPLE},
    {"start", TOKEN_START},
    {"statistics", TOKEN_STATISTICS},
    {"storage", TOKEN_STORAGE},
    {"stored", TOKEN_STORED},
    {"table", TOKEN_TABLE},
    {"tablespace", TOKEN_TABLESPACE},
    {"temp", TOKEN_TEMP},
    {"temporary", TOKEN_TEMPORARY},
    {"to", TOKEN_TO},
    {"unique", TOKEN_UNIQUE},
    {"unlogged", TOKEN_UNLOGGED},
    {"update", TOKEN_UPDATE},
    {"using", TOKEN_USING},
    {"values", TOKEN_VALUES},
    {"virtual", TOKEN_VIRTUAL},
    {"where", TOKEN_WHERE},
    {"with", TOKEN_WITH},
    {"without", TOKEN_WITHOUT},
};

static const int keyword_count = sizeof(keywords) / sizeof(keywords[0]);

/* Helper functions */
static bool is_at_end(Lexer *lexer) {
    return *lexer->current == '\0';
}

static char advance(Lexer *lexer) {
    if (is_at_end(lexer)) {
        return '\0';
    }
    lexer->current++;
    lexer->column++;
    return lexer->current[-1];
}

static char peek(Lexer *lexer) {
    return *lexer->current;
}

static char peek_next(Lexer *lexer) {
    if (is_at_end(lexer)) {
        return '\0';
    }
    return lexer->current[1];
}

static bool match(Lexer *lexer, char expected) {
    if (is_at_end(lexer)) {
        return false;
    }
    if (*lexer->current != expected) {
        return false;
    }
    lexer->current++;
    lexer->column++;
    return true;
}

static void skip_whitespace(Lexer *lexer) {
    while (true) {
        char c = peek(lexer);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance(lexer);
                break;
            case '\n':
                lexer->line++;
                lexer->column = 0;
                advance(lexer);
                break;
            case '-':
                /* SQL comment -- */
                if (peek_next(lexer) == '-') {
                    while (peek(lexer) != '\n' && !is_at_end(lexer)) {
                        advance(lexer);
                    }
                } else {
                    return;
                }
                break;
            case '/':
                /* C-style comment */
                if (peek_next(lexer) == '*') {
                    advance(lexer); /* / */
                    advance(lexer); /* * */
                    while (!is_at_end(lexer)) {
                        if (peek(lexer) == '*' && peek_next(lexer) == '/') {
                            advance(lexer); /* * */
                            advance(lexer); /* / */
                            break;
                        }
                        if (peek(lexer) == '\n') {
                            lexer->line++;
                            lexer->column = 0;
                        }
                        advance(lexer);
                    }
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

static Token make_token(Lexer *lexer, TokenType type) {
    Token token;
    token.type = type;
    token.line = lexer->line;
    token.column = lexer->column - (int)(lexer->current - lexer->start);
    token.length = (size_t)(lexer->current - lexer->start);
    token.lexeme = strndup(lexer->start, token.length);
    return token;
}

static Token error_token(Lexer *lexer, const char *message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.line = lexer->line;
    token.column = lexer->column;
    token.length = strlen(message);
    token.lexeme = strdup(message);
    lexer->had_error = true;
    if (lexer->error_message) {
        free(lexer->error_message);
    }
    lexer->error_message = strdup(message);
    return token;
}

/* Check if keyword using binary search */
bool is_keyword(const char *text, TokenType *type) {
    char *lower = str_to_lower(text);
    if (!lower) {
        return false;
    }

    int left = 0;
    int right = keyword_count - 1;

    while (left <= right) {
        int mid = (left + right) / 2;
        int cmp = strcmp(lower, keywords[mid].keyword);

        if (cmp == 0) {
            *type = keywords[mid].type;
            free(lower);
            return true;
        } else if (cmp < 0) {
            right = mid - 1;
        } else {
            left = mid + 1;
        }
    }

    free(lower);
    return false;
}

static Token identifier(Lexer *lexer) {
    while (isalnum(peek(lexer)) || peek(lexer) == '_') {
        advance(lexer);
    }

    /* Check if it's a keyword */
    size_t length = (size_t)(lexer->current - lexer->start);
    char *text = strndup(lexer->start, length);
    TokenType type;

    if (is_keyword(text, &type)) {
        free(text);
        return make_token(lexer, type);
    }

    free(text);
    return make_token(lexer, TOKEN_IDENTIFIER);
}

static Token quoted_identifier(Lexer *lexer) {
    /* Skip opening quote */
    advance(lexer);

    while (!is_at_end(lexer) && peek(lexer) != '"') {
        if (peek(lexer) == '"' && peek_next(lexer) == '"') {
            /* Escaped quote */
            advance(lexer);
            advance(lexer);
        } else {
            if (peek(lexer) == '\n') {
                lexer->line++;
                lexer->column = 0;
            }
            advance(lexer);
        }
    }

    if (is_at_end(lexer)) {
        return error_token(lexer, "Unterminated quoted identifier");
    }

    /* Closing quote */
    advance(lexer);
    return make_token(lexer, TOKEN_IDENTIFIER);
}

static Token string_literal(Lexer *lexer) {
    /* Skip opening quote */
    advance(lexer);

    while (!is_at_end(lexer) && peek(lexer) != '\'') {
        if (peek(lexer) == '\'' && peek_next(lexer) == '\'') {
            /* Escaped quote */
            advance(lexer);
            advance(lexer);
        } else if (peek(lexer) == '\\') {
            /* Escape sequence */
            advance(lexer);
            if (!is_at_end(lexer)) {
                advance(lexer);
            }
        } else {
            if (peek(lexer) == '\n') {
                lexer->line++;
                lexer->column = 0;
            }
            advance(lexer);
        }
    }

    if (is_at_end(lexer)) {
        return error_token(lexer, "Unterminated string literal");
    }

    /* Closing quote */
    advance(lexer);
    return make_token(lexer, TOKEN_STRING_LITERAL);
}

static Token number(Lexer *lexer) {
    while (isdigit(peek(lexer))) {
        advance(lexer);
    }

    /* Decimal part */
    if (peek(lexer) == '.' && isdigit(peek_next(lexer))) {
        advance(lexer); /* . */
        while (isdigit(peek(lexer))) {
            advance(lexer);
        }
    }

    /* Exponent part */
    if (peek(lexer) == 'e' || peek(lexer) == 'E') {
        advance(lexer);
        if (peek(lexer) == '+' || peek(lexer) == '-') {
            advance(lexer);
        }
        while (isdigit(peek(lexer))) {
            advance(lexer);
        }
    }

    return make_token(lexer, TOKEN_NUMBER);
}

/* Initialize lexer */
void lexer_init(Lexer *lexer, const char *source) {
    lexer->source = source;
    lexer->start = source;
    lexer->current = source;
    lexer->line = 1;
    lexer->column = 1;
    lexer->had_error = false;
    lexer->error_message = NULL;
}

/* Get next token */
Token lexer_next_token(Lexer *lexer) {
    skip_whitespace(lexer);

    lexer->start = lexer->current;

    if (is_at_end(lexer)) {
        return make_token(lexer, TOKEN_EOF);
    }

    char c = advance(lexer);

    /* Identifiers and keywords */
    if (isalpha(c) || c == '_') {
        return identifier(lexer);
    }

    /* Numbers */
    if (isdigit(c)) {
        return number(lexer);
    }

    /* Single-character and multi-character tokens */
    switch (c) {
        case '(': return make_token(lexer, TOKEN_LPAREN);
        case ')': return make_token(lexer, TOKEN_RPAREN);
        case ',': return make_token(lexer, TOKEN_COMMA);
        case ';': return make_token(lexer, TOKEN_SEMICOLON);
        case '.': return make_token(lexer, TOKEN_DOT);
        case '=': return make_token(lexer, TOKEN_EQUAL);
        case '[': return make_token(lexer, TOKEN_LBRACKET);
        case ']': return make_token(lexer, TOKEN_RBRACKET);
        case ':':
            if (match(lexer, ':')) {
                return make_token(lexer, TOKEN_COLONCOLON);
            }
            return error_token(lexer, "Unexpected character ':'");
        case '"':
            return quoted_identifier(lexer);
        case '\'':
            return string_literal(lexer);
    }

    char msg[64];
    snprintf(msg, sizeof(msg), "Unexpected character '%c'", c);
    return error_token(lexer, msg);
}

/* Free token memory */
void lexer_free_token(Token *token) {
    if (token && token->lexeme) {
        free(token->lexeme);
        token->lexeme = NULL;
    }
}

/* Get token type name */
const char *token_type_name(TokenType type) {
    switch (type) {
        case TOKEN_CREATE: return "CREATE";
        case TOKEN_TABLE: return "TABLE";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_STRING_LITERAL: return "STRING";
        case TOKEN_NUMBER: return "NUMBER";
        case TOKEN_LPAREN: return "(";
        case TOKEN_RPAREN: return ")";
        case TOKEN_COMMA: return ",";
        case TOKEN_SEMICOLON: return ";";
        case TOKEN_EOF: return "EOF";
        case TOKEN_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

/* Cleanup lexer */
void lexer_cleanup(Lexer *lexer) {
    if (lexer && lexer->error_message) {
        free(lexer->error_message);
        lexer->error_message = NULL;
    }
}
