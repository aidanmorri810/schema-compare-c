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

        /* Parse INHERITS clause */
        if (parser_match(parser, TOKEN_INHERITS)) {
            if (!parser_expect(parser, TOKEN_LPAREN, "Expected '(' after INHERITS")) {
                return NULL;
            }

            /* Count inherited tables */
            int capacity = 4;
            char **inherits = malloc(sizeof(char *) * capacity);
            int count = 0;

            do {
                if (!parser_check(parser, TOKEN_IDENTIFIER)) {
                    parser_error(parser, "Expected table name in INHERITS clause");
                    return NULL;
                }

                if (count >= capacity) {
                    capacity *= 2;
                    char **new_inherits = malloc(sizeof(char *) * capacity);
                    if (!new_inherits) {
                        parser_error(parser, "Out of memory");
                        return NULL;
                    }
                    memcpy(new_inherits, inherits, sizeof(char *) * count);
                    free(inherits);
                    inherits = new_inherits;
                }

                inherits[count++] = strdup(parser->current.lexeme);
                parser_advance(parser);
            } while (parser_match(parser, TOKEN_COMMA));

            if (!parser_expect(parser, TOKEN_RPAREN, "Expected ')' after INHERITS list")) {
                return NULL;
            }

            stmt->table_def.regular.inherits = inherits;
            stmt->table_def.regular.inherits_count = count;
        }

        /* Parse PARTITION BY clause */
        stmt->partition_by = parse_partition_by(parser);

        /* Parse USING method (for access method) */
        if (parser_match(parser, TOKEN_USING)) {
            if (!parser_check(parser, TOKEN_IDENTIFIER)) {
                parser_error(parser, "Expected access method name after USING");
                return NULL;
            }
            stmt->using_method = strdup(parser->current.lexeme);
            parser_advance(parser);
        } else {
            stmt->using_method = NULL;
        }

        /* Parse WITH options or WITHOUT OIDS */
        stmt->with_options = NULL;
        stmt->without_oids = false;

        if (parser_match(parser, TOKEN_WITH)) {
            if (parser_match(parser, TOKEN_OIDS)) {
                /* WITH OIDS - deprecated but still parsed */
                stmt->without_oids = false;
            } else if (parser_check(parser, TOKEN_LPAREN)) {
                /* WITH (...) storage options */
                /* parse_with_options expects WITH to already be consumed, so we need to parse manually */
                if (!parser_expect(parser, TOKEN_LPAREN, "Expected '(' after WITH")) {
                    return NULL;
                }

                StorageParameterList *list = calloc(1, sizeof(StorageParameterList));
                if (!list) {
                    parser_error(parser, "Out of memory");
                    return NULL;
                }

                int capacity = 4;
                list->parameters = calloc(capacity, sizeof(StorageParameter));
                if (!list->parameters) {
                    parser_error(parser, "Out of memory");
                    return NULL;
                }
                list->count = 0;

                do {
                    if (!parser_check(parser, TOKEN_IDENTIFIER)) {
                        parser_error(parser, "Expected storage parameter name");
                        return NULL;
                    }

                    if (list->count >= capacity) {
                        capacity *= 2;
                        StorageParameter *new_params = calloc(capacity, sizeof(StorageParameter));
                        if (!new_params) {
                            parser_error(parser, "Out of memory");
                            return NULL;
                        }
                        memcpy(new_params, list->parameters, sizeof(StorageParameter) * list->count);
                        free(list->parameters);
                        list->parameters = new_params;
                    }

                    list->parameters[list->count].name = strdup(parser->current.lexeme);
                    parser_advance(parser);

                    if (!parser_expect(parser, TOKEN_EQUAL, "Expected '=' after parameter name")) {
                        return NULL;
                    }

                    /* Parameter value can be identifier, number, or string */
                    if (parser_check(parser, TOKEN_IDENTIFIER) ||
                        parser_check(parser, TOKEN_NUMBER) ||
                        parser_check(parser, TOKEN_STRING_LITERAL)) {
                        list->parameters[list->count].value = strdup(parser->current.lexeme);
                        parser_advance(parser);
                    } else {
                        parser_error(parser, "Expected parameter value");
                        return NULL;
                    }

                    list->count++;
                } while (parser_match(parser, TOKEN_COMMA));

                if (!parser_expect(parser, TOKEN_RPAREN, "Expected ')' after WITH options")) {
                    return NULL;
                }

                stmt->with_options = list;
            } else {
                parser_error(parser, "Expected OIDS or '(' after WITH");
                return NULL;
            }
        } else if (parser_match(parser, TOKEN_WITHOUT)) {
            if (!parser_expect(parser, TOKEN_OIDS, "Expected OIDS after WITHOUT")) {
                return NULL;
            }
            stmt->without_oids = true;
        }

        /* Parse ON COMMIT clause */
        stmt->has_on_commit = false;
        if (parser_match(parser, TOKEN_ON)) {
            if (!parser_expect(parser, TOKEN_COMMIT, "Expected COMMIT after ON")) {
                return NULL;
            }

            if (parser_match(parser, TOKEN_PRESERVE)) {
                if (!parser_expect(parser, TOKEN_ROWS, "Expected ROWS after PRESERVE")) {
                    return NULL;
                }
                stmt->on_commit = ON_COMMIT_PRESERVE_ROWS;
                stmt->has_on_commit = true;
            } else if (parser_match(parser, TOKEN_DELETE)) {
                if (!parser_expect(parser, TOKEN_ROWS, "Expected ROWS after DELETE")) {
                    return NULL;
                }
                stmt->on_commit = ON_COMMIT_DELETE_ROWS;
                stmt->has_on_commit = true;
            } else if (parser_match(parser, TOKEN_DROP)) {
                stmt->on_commit = ON_COMMIT_DROP;
                stmt->has_on_commit = true;
            } else {
                parser_error(parser, "Expected PRESERVE ROWS, DELETE ROWS, or DROP after ON COMMIT");
                return NULL;
            }
        }

        /* Parse TABLESPACE clause */
        if (parser_match(parser, TOKEN_TABLESPACE)) {
            if (!parser_check(parser, TOKEN_IDENTIFIER)) {
                parser_error(parser, "Expected tablespace name after TABLESPACE");
                return NULL;
            }
            stmt->tablespace_name = strdup(parser->current.lexeme);
            parser_advance(parser);
        } else {
            stmt->tablespace_name = NULL;
        }
    }

    /* Consume optional semicolon to terminate the statement */
    parser_match(parser, TOKEN_SEMICOLON);

    return stmt;
}

