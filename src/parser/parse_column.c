#include "parser.h"
#include "sc_memory.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

/* Parse column definition */
ColumnDef *parse_column_def(Parser *parser) {
    ColumnDef *col = column_def_alloc(parser->memory_ctx);
    if (!col) {
        parser_error(parser, "Out of memory");
        return NULL;
    }

    /* Parse column name */
    /* Accept IDENTIFIER or certain keywords that can be used as column names */
    if (!parser_check(parser, TOKEN_IDENTIFIER) &&
        !parser_check(parser, TOKEN_COMMENTS)) {  /* COMMENTS is a non-reserved keyword */
        parser_error(parser, "Expected column name");
        return NULL;
    }

    col->column_name = strdup(parser->current.lexeme);
    parser_advance(parser);

    /* Parse data type */
    col->data_type = parse_data_type(parser);
    if (!col->data_type) {
        parser_error(parser, "Expected data type after column name");
        return NULL;
    }

    /* Initialize optional fields */
    col->has_storage = false;
    col->compression_method = NULL;
    col->collation = NULL;
    col->constraints = NULL;

    /* Parse optional column attributes */
    while (true) {
        if (parser_match(parser, TOKEN_COLLATE)) {
            if (!parser_check(parser, TOKEN_IDENTIFIER)) {
                parser_error(parser, "Expected collation name after COLLATE");
                return NULL;
            }
            col->collation = strdup(parser->current.lexeme);
            parser_advance(parser);
        } else if (parser_match(parser, TOKEN_STORAGE)) {
            col->has_storage = true;
            if (parser_match(parser, TOKEN_PLAIN)) {
                col->storage_type = STORAGE_TYPE_PLAIN;
            } else if (parser_match(parser, TOKEN_EXTERNAL)) {
                col->storage_type = STORAGE_TYPE_EXTERNAL;
            } else if (parser_match(parser, TOKEN_EXTENDED)) {
                col->storage_type = STORAGE_TYPE_EXTENDED;
            } else if (parser_match(parser, TOKEN_MAIN)) {
                col->storage_type = STORAGE_TYPE_MAIN;
            } else if (parser_match(parser, TOKEN_DEFAULT)) {
                col->storage_type = STORAGE_TYPE_DEFAULT;
            } else {
                parser_error(parser, "Expected storage type (PLAIN, EXTERNAL, EXTENDED, MAIN, DEFAULT)");
                return NULL;
            }
        } else if (parser_match(parser, TOKEN_COMPRESSION)) {
            if (!parser_check(parser, TOKEN_IDENTIFIER)) {
                parser_error(parser, "Expected compression method after COMPRESSION");
                return NULL;
            }
            col->compression_method = strdup(parser->current.lexeme);
            parser_advance(parser);
        } else if (parser_check(parser, TOKEN_CONSTRAINT) ||
                   parser_check(parser, TOKEN_NOT) ||
                   parser_check(parser, TOKEN_NULL) ||
                   parser_check(parser, TOKEN_CHECK) ||
                   parser_check(parser, TOKEN_DEFAULT) ||
                   parser_check(parser, TOKEN_GENERATED) ||
                   parser_check(parser, TOKEN_UNIQUE) ||
                   parser_check(parser, TOKEN_PRIMARY) ||
                   parser_check(parser, TOKEN_REFERENCES)) {
            /* Parse column constraints */
            ColumnConstraint *constraint = parse_column_constraint(parser);
            if (constraint) {
                /* Add to constraint list */
                constraint->next = col->constraints;
                col->constraints = constraint;
            }
        } else {
            /* No more column attributes */
            break;
        }
    }

    return col;
}

/* Parse data type */
char *parse_data_type(Parser *parser) {
    if (!parser_check(parser, TOKEN_IDENTIFIER)) {
        return NULL;
    }

    /* Build data type string (may include modifiers like length, precision) */
    StringBuilder *sb = sb_create();
    if (!sb) {
        return NULL;
    }

    /* Base type name */
    sb_append(sb, parser->current.lexeme);
    parser_advance(parser);

    /* Check for schema-qualified type (schema.typename) */
    if (parser_match(parser, TOKEN_DOT)) {
        sb_append(sb, ".");
        if (!parser_check(parser, TOKEN_IDENTIFIER)) {
            parser_error(parser, "Expected type name after schema qualifier");
            sb_free(sb);
            return NULL;
        }
        sb_append(sb, parser->current.lexeme);
        parser_advance(parser);
    }

    /* Check for type modifiers: (length) or (precision, scale) */
    if (parser_match(parser, TOKEN_LPAREN)) {
        sb_append(sb, "(");

        /* First number */
        if (!parser_check(parser, TOKEN_NUMBER)) {
            parser_error(parser, "Expected number in type modifier");
            sb_free(sb);
            return NULL;
        }
        sb_append(sb, parser->current.lexeme);
        parser_advance(parser);

        /* Optional second number (for precision/scale) */
        if (parser_match(parser, TOKEN_COMMA)) {
            sb_append(sb, ",");
            if (!parser_check(parser, TOKEN_NUMBER)) {
                parser_error(parser, "Expected number after comma in type modifier");
                sb_free(sb);
                return NULL;
            }
            sb_append(sb, parser->current.lexeme);
            parser_advance(parser);
        }

        if (!parser_expect(parser, TOKEN_RPAREN, "Expected ')' after type modifier")) {
            sb_free(sb);
            return NULL;
        }
        sb_append(sb, ")");
    }

    /* Check for array notation [] */
    while (parser_match(parser, TOKEN_LBRACKET)) {
        sb_append(sb, "[");

        /* Optional array size */
        if (parser_check(parser, TOKEN_NUMBER)) {
            sb_append(sb, parser->current.lexeme);
            parser_advance(parser);
        }

        if (!parser_expect(parser, TOKEN_RBRACKET, "Expected ']' in array type")) {
            sb_free(sb);
            return NULL;
        }
        sb_append(sb, "]");
    }

    char *type_str = sb_to_string(sb);
    sb_free(sb);

    /* Return directly - no need to copy since stmt owns it */
    return type_str;
}
