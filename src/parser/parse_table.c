#include "parser.h"
#include "sc_memory.h"
#include <stdlib.h>
#include <string.h>

/* Parse table element list (columns, constraints, LIKE clauses) */
TableElement *parse_table_element_list(Parser *parser) {
    if (!parser_expect(parser, TOKEN_LPAREN, "Expected '(' after table name")) {
        return NULL;
    }

    TableElement *head = NULL;
    TableElement *tail = NULL;

    while (!parser_check(parser, TOKEN_RPAREN) && !parser_check(parser, TOKEN_EOF)) {
        TableElement *elem = parse_table_element(parser);
        if (!elem) {
            parser_synchronize(parser);
            continue;
        }

        if (!head) {
            head = elem;
            tail = elem;
        } else {
            tail->next = elem;
            tail = elem;
        }

        if (!parser_match(parser, TOKEN_COMMA)) {
            break;
        }
    }

    if (!parser_expect(parser, TOKEN_RPAREN, "Expected ')' after table elements")) {
        return NULL;
    }

    return head;
}

/* Parse single table element */
TableElement *parse_table_element(Parser *parser) {
    TableElement *elem = table_element_alloc(parser->memory_ctx);
    if (!elem) {
        parser_error(parser, "Out of memory");
        return NULL;
    }

    /* Check for LIKE clause */
    if (parser_match(parser, TOKEN_LIKE)) {
        elem->type = TABLE_ELEM_LIKE;
        LikeClause *like = parse_like_clause(parser);
        if (!like) {
            return NULL;
        }
        elem->elem.like = *like;
        free(like); /* Free the temporary struct (contents were copied) */
        return elem;
    }

    /* Check for table constraint (starts with CONSTRAINT or constraint keyword) */
    if (parser_check(parser, TOKEN_CONSTRAINT) ||
        parser_check(parser, TOKEN_CHECK) ||
        parser_check(parser, TOKEN_UNIQUE) ||
        parser_check(parser, TOKEN_PRIMARY) ||
        parser_check(parser, TOKEN_FOREIGN) ||
        parser_check(parser, TOKEN_EXCLUDE)) {

        elem->type = TABLE_ELEM_TABLE_CONSTRAINT;
        elem->elem.table_constraint = parse_table_constraint(parser);
        if (!elem->elem.table_constraint) {
            return NULL;
        }
        return elem;
    }

    /* Otherwise, it's a column definition */
    elem->type = TABLE_ELEM_COLUMN;
    ColumnDef *col = parse_column_def(parser);
    if (!col) {
        return NULL;
    }
    elem->elem.column = *col;
    free(col); /* Free the temporary struct (contents were copied) */
    return elem;
}

/* Parse LIKE clause */
LikeClause *parse_like_clause(Parser *parser) {
    LikeClause *like = malloc(sizeof(LikeClause));
    if (!like) {
        parser_error(parser, "Out of memory");
        return NULL;
    }

    /* Parse source table name */
    if (!parser_check(parser, TOKEN_IDENTIFIER)) {
        parser_error(parser, "Expected table name after LIKE");
        free(like);
        return NULL;
    }

    like->source_table = strdup(parser->current.lexeme);
    parser_advance(parser);

    /* Parse like options (INCLUDING/EXCLUDING) */
    like->options = NULL;
    like->option_count = 0;

    int capacity = 4;
    LikeOption *options = malloc(sizeof(LikeOption) * capacity);
    if (!options) {
        parser_error(parser, "Out of memory");
        return NULL;
    }

    while (parser_match(parser, TOKEN_INCLUDING) || parser_match(parser, TOKEN_EXCLUDING)) {
        bool including = parser->previous.type == TOKEN_INCLUDING;

        /* Parse option type */
        LikeOptionType opt_type;
        if (parser_match(parser, TOKEN_COMMENTS)) {
            opt_type = LIKE_OPT_COMMENTS;
        } else if (parser_match(parser, TOKEN_COMPRESSION)) {
            opt_type = LIKE_OPT_COMPRESSION;
        } else if (parser_match(parser, TOKEN_CONSTRAINTS)) {
            opt_type = LIKE_OPT_CONSTRAINTS;
        } else if (parser_match(parser, TOKEN_DEFAULTS)) {
            opt_type = LIKE_OPT_DEFAULTS;
        } else if (parser_match(parser, TOKEN_GENERATED)) {
            opt_type = LIKE_OPT_GENERATED;
        } else if (parser_match(parser, TOKEN_IDENTITY)) {
            opt_type = LIKE_OPT_IDENTITY;
        } else if (parser_match(parser, TOKEN_INDEXES)) {
            opt_type = LIKE_OPT_INDEXES;
        } else if (parser_match(parser, TOKEN_STATISTICS)) {
            opt_type = LIKE_OPT_STATISTICS;
        } else if (parser_match(parser, TOKEN_STORAGE)) {
            opt_type = LIKE_OPT_STORAGE;
        } else if (parser_match(parser, TOKEN_ALL)) {
            opt_type = LIKE_OPT_ALL;
        } else {
            parser_error(parser, "Expected LIKE option after INCLUDING/EXCLUDING");
            return NULL;
        }

        /* Expand array if needed */
        if (like->option_count >= capacity) {
            capacity *= 2;
            LikeOption *new_options = realloc(options, sizeof(LikeOption) * capacity);
            if (!new_options) {
                parser_error(parser, "Out of memory");
                free(options);
                free(like->source_table);
                free(like);
                return NULL;
            }
            options = new_options;
        }

        options[like->option_count].option = opt_type;
        options[like->option_count].including = including;
        like->option_count++;
    }

    like->options = options;
    return like;
}
