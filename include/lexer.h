#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h>
#include <stddef.h>

/* Token types */
typedef enum {
    /* Keywords - Table related */
    TOKEN_CREATE,
    TOKEN_ALTER,
    TOKEN_TABLE,
    TOKEN_TEMPORARY,
    TOKEN_TEMP,
    TOKEN_UNLOGGED,
    TOKEN_IF,
    TOKEN_NOT,
    TOKEN_EXISTS,
    TOKEN_OF,
    TOKEN_PARTITION,
    TOKEN_FOR,
    TOKEN_VALUES,
    TOKEN_IN,
    TOKEN_FROM,
    TOKEN_TO,
    TOKEN_WITH,
    TOKEN_MODULUS,
    TOKEN_REMAINDER,
    TOKEN_DEFAULT,

    /* Keywords - Constraints */
    TOKEN_CONSTRAINT,
    TOKEN_CHECK,
    TOKEN_UNIQUE,
    TOKEN_PRIMARY,
    TOKEN_KEY,
    TOKEN_REFERENCES,
    TOKEN_FOREIGN,
    TOKEN_NULL,
    TOKEN_GENERATED,
    TOKEN_ALWAYS,
    TOKEN_AS,
    TOKEN_IDENTITY,
    TOKEN_BY,
    TOKEN_STORED,
    TOKEN_VIRTUAL,
    TOKEN_EXCLUDE,
    TOKEN_MATCH,
    TOKEN_FULL,
    TOKEN_PARTIAL,
    TOKEN_SIMPLE,
    TOKEN_DEFERRABLE,
    TOKEN_INITIALLY,
    TOKEN_DEFERRED,
    TOKEN_IMMEDIATE,
    TOKEN_ENFORCED,

    /* Keywords - Actions */
    TOKEN_CASCADE,
    TOKEN_RESTRICT,
    TOKEN_ACTION,
    TOKEN_SET,
    TOKEN_NO,
    TOKEN_ON,
    TOKEN_DELETE,
    TOKEN_UPDATE,
    TOKEN_COMMIT,
    TOKEN_PRESERVE,
    TOKEN_DROP,
    TOKEN_ROWS,

    /* Keywords - Column related */
    TOKEN_COLLATE,
    TOKEN_STORAGE,
    TOKEN_PLAIN,
    TOKEN_EXTERNAL,
    TOKEN_EXTENDED,
    TOKEN_MAIN,
    TOKEN_COMPRESSION,

    /* Keywords - Table options */
    TOKEN_INHERITS,
    TOKEN_LIKE,
    TOKEN_INCLUDING,
    TOKEN_EXCLUDING,
    TOKEN_USING,
    TOKEN_WHERE,
    TOKEN_TABLESPACE,
    TOKEN_WITHOUT,
    TOKEN_OIDS,
    TOKEN_GLOBAL,
    TOKEN_LOCAL,

    /* Keywords - Partition related */
    TOKEN_RANGE,
    TOKEN_LIST,
    TOKEN_HASH,
    TOKEN_MINVALUE,
    TOKEN_MAXVALUE,

    /* Keywords - Index/Unique related */
    TOKEN_NULLS,
    TOKEN_DISTINCT,
    TOKEN_FIRST,
    TOKEN_LAST,
    TOKEN_ASC,
    TOKEN_DESC,
    TOKEN_INCLUDE,
    TOKEN_OVERLAPS,
    TOKEN_PERIOD,

    /* Keywords - Other */
    TOKEN_NO_INHERIT,
    TOKEN_INHERIT,
    TOKEN_COMMENTS,
    TOKEN_CONSTRAINTS,
    TOKEN_DEFAULTS,
    TOKEN_INDEXES,
    TOKEN_STATISTICS,
    TOKEN_ALL,

    /* Literals */
    TOKEN_IDENTIFIER,
    TOKEN_STRING_LITERAL,
    TOKEN_NUMBER,

    /* Operators and punctuation */
    TOKEN_LPAREN,              /* ( */
    TOKEN_RPAREN,              /* ) */
    TOKEN_COMMA,               /* , */
    TOKEN_SEMICOLON,           /* ; */
    TOKEN_DOT,                 /* . */
    TOKEN_EQUAL,               /* = */
    TOKEN_COLONCOLON,          /* :: */
    TOKEN_LBRACKET,            /* [ */
    TOKEN_RBRACKET,            /* ] */

    /* Special */
    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

/* Token structure */
typedef struct {
    TokenType type;
    char *lexeme;              /* actual text */
    size_t length;             /* length of lexeme */
    int line;                  /* line number */
    int column;                /* column number */
} Token;

/* Lexer state */
typedef struct {
    const char *source;        /* input source code */
    const char *start;         /* start of current token */
    const char *current;       /* current position */
    int line;                  /* current line */
    int column;                /* current column */
    bool had_error;            /* error flag */
    char *error_message;       /* last error message */
} Lexer;

/* Lexer API */
void lexer_init(Lexer *lexer, const char *source);
Token lexer_next_token(Lexer *lexer);
void lexer_free_token(Token *token);
const char *token_type_name(TokenType type);
bool is_keyword(const char *text, TokenType *type);
void lexer_cleanup(Lexer *lexer);

#endif /* LEXER_H */
