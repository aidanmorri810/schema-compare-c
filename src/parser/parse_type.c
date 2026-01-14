#include "parser.h"
#include "pg_create_type.h"
#include "sc_memory.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Forward declarations */
static EnumTypeDef *parse_enum_type_def(Parser *parser);
static CompositeTypeDef *parse_composite_type_def(Parser *parser);
static RangeTypeDef *parse_range_type_def(Parser *parser);
static BaseTypeDef *parse_base_type_def(Parser *parser);

/* Helper: Strip surrounding quotes from string literal */
static char *strip_quotes(const char *str) {
    if (!str) return NULL;

    size_t len = strlen(str);
    if (len < 2) return strdup(str);

    /* Check if surrounded by single quotes */
    if (str[0] == '\'' && str[len - 1] == '\'') {
        char *result = malloc(len - 1);
        if (!result) return NULL;
        memcpy(result, str + 1, len - 2);
        result[len - 2] = '\0';
        return result;
    }

    /* Check if surrounded by double quotes */
    if (str[0] == '"' && str[len - 1] == '"') {
        char *result = malloc(len - 1);
        if (!result) return NULL;
        memcpy(result, str + 1, len - 2);
        result[len - 2] = '\0';
        return result;
    }

    /* No quotes to strip */
    return strdup(str);
}

/* Parse CREATE TYPE statement */
CreateTypeStmt *parser_parse_create_type(Parser *parser) {
    if (!parser_expect(parser, TOKEN_CREATE, "Expected CREATE")) {
        return NULL;
    }

    if (!parser_expect(parser, TOKEN_TYPE, "Expected TYPE")) {
        return NULL;
    }

    CreateTypeStmt *stmt = malloc(sizeof(CreateTypeStmt));
    if (!stmt) {
        parser_error(parser, "Out of memory");
        return NULL;
    }

    /* Initialize */
    memset(stmt, 0, sizeof(CreateTypeStmt));
    stmt->if_not_exists = false;

    /* Check for IF NOT EXISTS (future PostgreSQL support) */
    if (parser_match(parser, TOKEN_IF)) {
        if (!parser_expect(parser, TOKEN_NOT, "Expected NOT after IF")) {
            return NULL;
        }
        if (!parser_expect(parser, TOKEN_EXISTS, "Expected EXISTS after IF NOT")) {
            return NULL;
        }
        stmt->if_not_exists = true;
    }

    /* Parse type name */
    if (!parser_check(parser, TOKEN_IDENTIFIER)) {
        parser_error(parser, "Expected type name");
        return NULL;
    }
    stmt->type_name = strdup(parser->current.lexeme);
    parser_advance(parser);

    /* Check for schema-qualified name */
    if (parser_match(parser, TOKEN_DOT)) {
        /* Previous identifier was schema, get actual type name */
        if (!parser_check(parser, TOKEN_IDENTIFIER)) {
            parser_error(parser, "Expected type name after '.'");
            return NULL;
        }
        /* Combine schema.type_name */
        char *schema = stmt->type_name;
        char *type_name = parser->current.lexeme;
        size_t len = strlen(schema) + strlen(type_name) + 2;
        stmt->type_name = malloc(len);
        snprintf(stmt->type_name, len, "%s.%s", schema, type_name);
        parser_advance(parser);
    }

    /* Determine type variant */
    if (parser_match(parser, TOKEN_AS)) {
        /* Could be ENUM, COMPOSITE, or RANGE */
        if (parser_match(parser, TOKEN_ENUM)) {
            /* ENUM type */
            stmt->variant = TYPE_VARIANT_ENUM;
            EnumTypeDef *enum_def = parse_enum_type_def(parser);
            if (!enum_def) {
                return NULL;
            }
            stmt->type_def.enum_def = *enum_def;
            free(enum_def);
        } else if (parser_match(parser, TOKEN_RANGE)) {
            /* RANGE type */
            stmt->variant = TYPE_VARIANT_RANGE;
            RangeTypeDef *range_def = parse_range_type_def(parser);
            if (!range_def) {
                return NULL;
            }
            stmt->type_def.range_def = *range_def;
            free(range_def);
        } else if (parser_check(parser, TOKEN_LPAREN)) {
            /* COMPOSITE type */
            stmt->variant = TYPE_VARIANT_COMPOSITE;
            CompositeTypeDef *comp_def = parse_composite_type_def(parser);
            if (!comp_def) {
                return NULL;
            }
            stmt->type_def.composite_def = *comp_def;
            free(comp_def);
        } else {
            parser_error(parser, "Expected ENUM, RANGE, or '(' after AS");
            return NULL;
        }
    } else if (parser_check(parser, TOKEN_LPAREN)) {
        /* BASE type (parameters in parentheses) */
        stmt->variant = TYPE_VARIANT_BASE;
        BaseTypeDef *base_def = parse_base_type_def(parser);
        if (!base_def) {
            return NULL;
        }
        stmt->type_def.base_def = *base_def;
        free(base_def);
    } else {
        parser_error(parser, "Expected AS or '(' after type name");
        return NULL;
    }

    return stmt;
}

/* Parse ENUM type definition: ('label1', 'label2', ...) */
static EnumTypeDef *parse_enum_type_def(Parser *parser) {
    EnumTypeDef *enum_def = malloc(sizeof(EnumTypeDef));
    if (!enum_def) {
        parser_error(parser, "Out of memory");
        return NULL;
    }

    enum_def->labels = NULL;
    enum_def->label_count = 0;

    if (!parser_expect(parser, TOKEN_LPAREN, "Expected '(' after ENUM")) {
        free(enum_def);
        return NULL;
    }

    /* Count labels first */
    int capacity = 8;
    char **labels = malloc(capacity * sizeof(char *));
    if (!labels) {
        parser_error(parser, "Out of memory");
        free(enum_def);
        return NULL;
    }

    while (!parser_check(parser, TOKEN_RPAREN) && !parser_check(parser, TOKEN_EOF)) {
        if (!parser_check(parser, TOKEN_STRING_LITERAL)) {
            parser_error(parser, "Expected string literal for enum label");
            free(enum_def);
            return NULL;
        }

        /* Resize if needed */
        if (enum_def->label_count >= capacity) {
            capacity *= 2;
            labels = realloc(labels, capacity * sizeof(char *));
            if (!labels) {
                parser_error(parser, "Out of memory");
                free(enum_def);
                return NULL;
            }
        }

        /* Strip quotes from the string literal */
        labels[enum_def->label_count++] = strip_quotes(parser->current.lexeme);
        parser_advance(parser);

        if (!parser_match(parser, TOKEN_COMMA)) {
            break;
        }
    }

    if (!parser_expect(parser, TOKEN_RPAREN, "Expected ')' after enum labels")) {
        free(enum_def);
        return NULL;
    }

    enum_def->labels = labels;
    return enum_def;
}

/* Parse COMPOSITE type definition: (attr1 type1 [COLLATE ...], attr2 type2, ...) */
static CompositeTypeDef *parse_composite_type_def(Parser *parser) {
    CompositeTypeDef *comp_def = malloc(sizeof(CompositeTypeDef));
    if (!comp_def) {
        parser_error(parser, "Out of memory");
        return NULL;
    }

    comp_def->attributes = NULL;
    comp_def->attribute_count = 0;

    if (!parser_expect(parser, TOKEN_LPAREN, "Expected '(' for composite type")) {
        free(comp_def);
        return NULL;
    }

    CompositeAttribute *head = NULL;
    CompositeAttribute *tail = NULL;

    while (!parser_check(parser, TOKEN_RPAREN) && !parser_check(parser, TOKEN_EOF)) {
        CompositeAttribute *attr = malloc(sizeof(CompositeAttribute));
        if (!attr) {
            parser_error(parser, "Out of memory");
            free(comp_def);
            return NULL;
        }

        memset(attr, 0, sizeof(CompositeAttribute));

        /* Parse attribute name */
        if (!parser_check(parser, TOKEN_IDENTIFIER)) {
            parser_error(parser, "Expected attribute name");
            free(comp_def);
            return NULL;
        }
        attr->attr_name = strdup(parser->current.lexeme);
        parser_advance(parser);

        /* Parse data type */
        attr->data_type = parse_data_type(parser);
        if (!attr->data_type) {
            free(comp_def);
            return NULL;
        }

        /* Optional COLLATE clause */
        if (parser_match(parser, TOKEN_COLLATE)) {
            if (!parser_check(parser, TOKEN_IDENTIFIER) && !parser_check(parser, TOKEN_STRING_LITERAL)) {
                parser_error(parser, "Expected collation name");
                free(comp_def);
                return NULL;
            }
            attr->collation = strip_quotes(parser->current.lexeme);
            parser_advance(parser);
        }

        /* Add to list */
        if (!head) {
            head = attr;
            tail = attr;
        } else {
            tail->next = attr;
            tail = attr;
        }
        comp_def->attribute_count++;

        if (!parser_match(parser, TOKEN_COMMA)) {
            break;
        }
    }

    if (!parser_expect(parser, TOKEN_RPAREN, "Expected ')' after composite type attributes")) {
        free(comp_def);
        return NULL;
    }

    comp_def->attributes = head;
    return comp_def;
}

/* Parse RANGE type definition: (SUBTYPE = type, ...) */
static RangeTypeDef *parse_range_type_def(Parser *parser) {
    RangeTypeDef *range_def = malloc(sizeof(RangeTypeDef));
    if (!range_def) {
        parser_error(parser, "Out of memory");
        return NULL;
    }

    memset(range_def, 0, sizeof(RangeTypeDef));

    if (!parser_expect(parser, TOKEN_LPAREN, "Expected '(' after RANGE")) {
        free(range_def);
        return NULL;
    }

    /* Parse range parameters */
    while (!parser_check(parser, TOKEN_RPAREN) && !parser_check(parser, TOKEN_EOF)) {
        if (parser_match(parser, TOKEN_SUBTYPE)) {
            if (!parser_expect(parser, TOKEN_EQUAL, "Expected '=' after SUBTYPE")) {
                free(range_def);
                return NULL;
            }
            if (!parser_check(parser, TOKEN_IDENTIFIER)) {
                parser_error(parser, "Expected subtype name");
                free(range_def);
                return NULL;
            }
            range_def->subtype = strdup(parser->current.lexeme);
            parser_advance(parser);
        } else if (parser_check(parser, TOKEN_IDENTIFIER) &&
                   strcmp(parser->current.lexeme, "subtype_opclass") == 0) {
            parser_advance(parser);
            if (!parser_expect(parser, TOKEN_EQUAL, "Expected '=' after SUBTYPE_OPCLASS")) {
                free(range_def);
                return NULL;
            }
            if (!parser_check(parser, TOKEN_IDENTIFIER)) {
                parser_error(parser, "Expected operator class name");
                free(range_def);
                return NULL;
            }
            range_def->subtype_opclass = strdup(parser->current.lexeme);
            parser_advance(parser);
        } else if (parser_match(parser, TOKEN_COLLATE)) {
            /* Note: In RANGE context, COLLATE is actually "collation =" */
            if (!parser_expect(parser, TOKEN_EQUAL, "Expected '=' after COLLATION")) {
                free(range_def);
                return NULL;
            }
            if (!parser_check(parser, TOKEN_IDENTIFIER) && !parser_check(parser, TOKEN_STRING_LITERAL)) {
                parser_error(parser, "Expected collation name");
                free(range_def);
                return NULL;
            }
            range_def->collation = strdup(parser->current.lexeme);
            parser_advance(parser);
        } else if (parser_match(parser, TOKEN_CANONICAL)) {
            if (!parser_expect(parser, TOKEN_EQUAL, "Expected '=' after CANONICAL")) {
                free(range_def);
                return NULL;
            }
            if (!parser_check(parser, TOKEN_IDENTIFIER)) {
                parser_error(parser, "Expected canonical function name");
                free(range_def);
                return NULL;
            }
            range_def->canonical_function = strdup(parser->current.lexeme);
            parser_advance(parser);
        } else if (parser_check(parser, TOKEN_IDENTIFIER) &&
                   strcmp(parser->current.lexeme, "subtype_diff") == 0) {
            parser_advance(parser);
            if (!parser_expect(parser, TOKEN_EQUAL, "Expected '=' after SUBTYPE_DIFF")) {
                free(range_def);
                return NULL;
            }
            if (!parser_check(parser, TOKEN_IDENTIFIER)) {
                parser_error(parser, "Expected subtype diff function name");
                free(range_def);
                return NULL;
            }
            range_def->subtype_diff_function = strdup(parser->current.lexeme);
            parser_advance(parser);
        } else if (parser_check(parser, TOKEN_IDENTIFIER) &&
                   strcmp(parser->current.lexeme, "multirange_type_name") == 0) {
            parser_advance(parser);
            if (!parser_expect(parser, TOKEN_EQUAL, "Expected '=' after MULTIRANGE_TYPE_NAME")) {
                free(range_def);
                return NULL;
            }
            if (!parser_check(parser, TOKEN_IDENTIFIER)) {
                parser_error(parser, "Expected multirange type name");
                free(range_def);
                return NULL;
            }
            range_def->multirange_type_name = strdup(parser->current.lexeme);
            parser_advance(parser);
        } else {
            parser_error(parser, "Unknown RANGE parameter");
            free(range_def);
            return NULL;
        }

        if (!parser_match(parser, TOKEN_COMMA)) {
            break;
        }
    }

    if (!parser_expect(parser, TOKEN_RPAREN, "Expected ')' after RANGE parameters")) {
        free(range_def);
        return NULL;
    }

    if (!range_def->subtype) {
        parser_error(parser, "RANGE type requires SUBTYPE parameter");
        free(range_def);
        return NULL;
    }

    return range_def;
}

/* Parse BASE type definition: (INPUT = func, OUTPUT = func, ...) */
static BaseTypeDef *parse_base_type_def(Parser *parser) {
    BaseTypeDef *base_def = malloc(sizeof(BaseTypeDef));
    if (!base_def) {
        parser_error(parser, "Out of memory");
        return NULL;
    }

    memset(base_def, 0, sizeof(BaseTypeDef));

    if (!parser_expect(parser, TOKEN_LPAREN, "Expected '(' for BASE type")) {
        free(base_def);
        return NULL;
    }

    /* Parse base type parameters */
    while (!parser_check(parser, TOKEN_RPAREN) && !parser_check(parser, TOKEN_EOF)) {
        if (parser_match(parser, TOKEN_INPUT)) {
            if (!parser_expect(parser, TOKEN_EQUAL, "Expected '=' after INPUT")) {
                free(base_def);
                return NULL;
            }
            if (!parser_check(parser, TOKEN_IDENTIFIER)) {
                parser_error(parser, "Expected input function name");
                free(base_def);
                return NULL;
            }
            base_def->input_function = strdup(parser->current.lexeme);
            parser_advance(parser);
        } else if (parser_match(parser, TOKEN_OUTPUT)) {
            if (!parser_expect(parser, TOKEN_EQUAL, "Expected '=' after OUTPUT")) {
                free(base_def);
                return NULL;
            }
            if (!parser_check(parser, TOKEN_IDENTIFIER)) {
                parser_error(parser, "Expected output function name");
                free(base_def);
                return NULL;
            }
            base_def->output_function = strdup(parser->current.lexeme);
            parser_advance(parser);
        } else if (parser_match(parser, TOKEN_RECEIVE)) {
            if (!parser_expect(parser, TOKEN_EQUAL, "Expected '=' after RECEIVE")) {
                free(base_def);
                return NULL;
            }
            if (!parser_check(parser, TOKEN_IDENTIFIER)) {
                parser_error(parser, "Expected receive function name");
                free(base_def);
                return NULL;
            }
            base_def->receive_function = strdup(parser->current.lexeme);
            parser_advance(parser);
        } else if (parser_match(parser, TOKEN_SEND)) {
            if (!parser_expect(parser, TOKEN_EQUAL, "Expected '=' after SEND")) {
                free(base_def);
                return NULL;
            }
            if (!parser_check(parser, TOKEN_IDENTIFIER)) {
                parser_error(parser, "Expected send function name");
                free(base_def);
                return NULL;
            }
            base_def->send_function = strdup(parser->current.lexeme);
            parser_advance(parser);
        } else if (parser_match(parser, TOKEN_TYPMOD_IN)) {
            if (!parser_expect(parser, TOKEN_EQUAL, "Expected '=' after TYPMOD_IN")) {
                free(base_def);
                return NULL;
            }
            if (!parser_check(parser, TOKEN_IDENTIFIER)) {
                parser_error(parser, "Expected typmod_in function name");
                free(base_def);
                return NULL;
            }
            base_def->typmod_in_function = strdup(parser->current.lexeme);
            parser_advance(parser);
        } else if (parser_match(parser, TOKEN_TYPMOD_OUT)) {
            if (!parser_expect(parser, TOKEN_EQUAL, "Expected '=' after TYPMOD_OUT")) {
                free(base_def);
                return NULL;
            }
            if (!parser_check(parser, TOKEN_IDENTIFIER)) {
                parser_error(parser, "Expected typmod_out function name");
                free(base_def);
                return NULL;
            }
            base_def->typmod_out_function = strdup(parser->current.lexeme);
            parser_advance(parser);
        } else if (parser_match(parser, TOKEN_ANALYZE)) {
            if (!parser_expect(parser, TOKEN_EQUAL, "Expected '=' after ANALYZE")) {
                free(base_def);
                return NULL;
            }
            if (!parser_check(parser, TOKEN_IDENTIFIER)) {
                parser_error(parser, "Expected analyze function name");
                free(base_def);
                return NULL;
            }
            base_def->analyze_function = strdup(parser->current.lexeme);
            parser_advance(parser);
        } else if (parser_match(parser, TOKEN_INTERNALLENGTH)) {
            if (!parser_expect(parser, TOKEN_EQUAL, "Expected '=' after INTERNALLENGTH")) {
                free(base_def);
                return NULL;
            }
            if (parser_match(parser, TOKEN_VARIABLE)) {
                base_def->is_variable_length = true;
                base_def->internallength = -1;
            } else if (parser_check(parser, TOKEN_NUMBER)) {
                base_def->internallength = atoi(parser->current.lexeme);
                base_def->is_variable_length = false;
                parser_advance(parser);
            } else {
                parser_error(parser, "Expected VARIABLE or number for INTERNALLENGTH");
                free(base_def);
                return NULL;
            }
            base_def->has_internallength = true;
        } else if (parser_match(parser, TOKEN_PASSEDBYVALUE)) {
            base_def->has_passedbyvalue = true;
            /* PASSEDBYVALUE can be just a flag or "= true/false" */
            if (parser_match(parser, TOKEN_EQUAL)) {
                if (parser_check(parser, TOKEN_IDENTIFIER)) {
                    if (strcmp(parser->current.lexeme, "true") == 0) {
                        base_def->passedbyvalue = true;
                    } else if (strcmp(parser->current.lexeme, "false") == 0) {
                        base_def->passedbyvalue = false;
                    } else {
                        parser_error(parser, "Expected true or false for PASSEDBYVALUE");
                        free(base_def);
                        return NULL;
                    }
                    parser_advance(parser);
                } else {
                    parser_error(parser, "Expected true or false for PASSEDBYVALUE");
                    free(base_def);
                    return NULL;
                }
            } else {
                base_def->passedbyvalue = true;
            }
        } else if (parser_match(parser, TOKEN_ALIGNMENT)) {
            if (!parser_expect(parser, TOKEN_EQUAL, "Expected '=' after ALIGNMENT")) {
                free(base_def);
                return NULL;
            }
            if (!parser_check(parser, TOKEN_IDENTIFIER)) {
                parser_error(parser, "Expected alignment value");
                free(base_def);
                return NULL;
            }
            /* Parse alignment: char, int2, int4, double */
            const char *align = parser->current.lexeme;
            if (strcmp(align, "char") == 0) {
                base_def->alignment = 'c';
            } else if (strcmp(align, "int2") == 0) {
                base_def->alignment = 's';
            } else if (strcmp(align, "int4") == 0) {
                base_def->alignment = 'i';
            } else if (strcmp(align, "double") == 0) {
                base_def->alignment = 'd';
            } else {
                parser_error(parser, "Invalid alignment value");
                free(base_def);
                return NULL;
            }
            base_def->has_alignment = true;
            parser_advance(parser);
        } else if (parser_match(parser, TOKEN_STORAGE)) {
            if (!parser_expect(parser, TOKEN_EQUAL, "Expected '=' after STORAGE")) {
                free(base_def);
                return NULL;
            }
            if (parser_match(parser, TOKEN_PLAIN)) {
                base_def->storage = 'p';
            } else if (parser_match(parser, TOKEN_EXTERNAL)) {
                base_def->storage = 'e';
            } else if (parser_match(parser, TOKEN_EXTENDED)) {
                base_def->storage = 'x';
            } else if (parser_match(parser, TOKEN_MAIN)) {
                base_def->storage = 'm';
            } else {
                parser_error(parser, "Expected PLAIN, EXTERNAL, EXTENDED, or MAIN for STORAGE");
                free(base_def);
                return NULL;
            }
            base_def->has_storage = true;
        } else if (parser_match(parser, TOKEN_LIKE)) {
            if (!parser_expect(parser, TOKEN_EQUAL, "Expected '=' after LIKE")) {
                free(base_def);
                return NULL;
            }
            if (!parser_check(parser, TOKEN_IDENTIFIER)) {
                parser_error(parser, "Expected type name for LIKE");
                free(base_def);
                return NULL;
            }
            base_def->like_type = strdup(parser->current.lexeme);
            parser_advance(parser);
        } else if (parser_check(parser, TOKEN_IDENTIFIER) &&
                   strcmp(parser->current.lexeme, "category") == 0) {
            parser_advance(parser);
            if (!parser_expect(parser, TOKEN_EQUAL, "Expected '=' after CATEGORY")) {
                free(base_def);
                return NULL;
            }
            if (!parser_check(parser, TOKEN_STRING_LITERAL) && !parser_check(parser, TOKEN_IDENTIFIER)) {
                parser_error(parser, "Expected category value");
                free(base_def);
                return NULL;
            }
            char *cat_str = strip_quotes(parser->current.lexeme);
            if (cat_str && strlen(cat_str) > 0) {
                base_def->category = cat_str[0];
                base_def->has_category = true;
            }
            free(cat_str);
            parser_advance(parser);
        } else if (parser_match(parser, TOKEN_PREFERRED)) {
            base_def->has_preferred = true;
            if (parser_match(parser, TOKEN_EQUAL)) {
                if (parser_check(parser, TOKEN_IDENTIFIER)) {
                    if (strcmp(parser->current.lexeme, "true") == 0) {
                        base_def->preferred = true;
                    } else if (strcmp(parser->current.lexeme, "false") == 0) {
                        base_def->preferred = false;
                    } else {
                        parser_error(parser, "Expected true or false for PREFERRED");
                        free(base_def);
                        return NULL;
                    }
                    parser_advance(parser);
                } else {
                    parser_error(parser, "Expected true or false for PREFERRED");
                    free(base_def);
                    return NULL;
                }
            } else {
                base_def->preferred = true;
            }
        } else if (parser_match(parser, TOKEN_DEFAULT)) {
            if (!parser_expect(parser, TOKEN_EQUAL, "Expected '=' after DEFAULT")) {
                free(base_def);
                return NULL;
            }
            if (!parser_check(parser, TOKEN_STRING_LITERAL) && !parser_check(parser, TOKEN_IDENTIFIER) && !parser_check(parser, TOKEN_NUMBER)) {
                parser_error(parser, "Expected default value");
                free(base_def);
                return NULL;
            }
            base_def->default_value = strdup(parser->current.lexeme);
            parser_advance(parser);
        } else if (parser_match(parser, TOKEN_ELEMENT)) {
            if (!parser_expect(parser, TOKEN_EQUAL, "Expected '=' after ELEMENT")) {
                free(base_def);
                return NULL;
            }
            if (!parser_check(parser, TOKEN_IDENTIFIER)) {
                parser_error(parser, "Expected element type name");
                free(base_def);
                return NULL;
            }
            base_def->element_type = strdup(parser->current.lexeme);
            parser_advance(parser);
        } else if (parser_match(parser, TOKEN_DELIMITER)) {
            if (!parser_expect(parser, TOKEN_EQUAL, "Expected '=' after DELIMITER")) {
                free(base_def);
                return NULL;
            }
            if (!parser_check(parser, TOKEN_STRING_LITERAL)) {
                parser_error(parser, "Expected string literal for DELIMITER");
                free(base_def);
                return NULL;
            }
            char *delim_str = strip_quotes(parser->current.lexeme);
            if (delim_str && strlen(delim_str) > 0) {
                base_def->delimiter = delim_str[0];
                base_def->has_delimiter = true;
            }
            free(delim_str);
            parser_advance(parser);
        } else if (parser_match(parser, TOKEN_COLLATABLE)) {
            if (!parser_expect(parser, TOKEN_EQUAL, "Expected '=' after COLLATABLE")) {
                free(base_def);
                return NULL;
            }
            if (!parser_check(parser, TOKEN_IDENTIFIER)) {
                parser_error(parser, "Expected true or false for COLLATABLE");
                free(base_def);
                return NULL;
            }
            base_def->collatable = strdup(parser->current.lexeme);
            parser_advance(parser);
        } else {
            parser_error(parser, "Unknown BASE type parameter");
            free(base_def);
            return NULL;
        }

        if (!parser_match(parser, TOKEN_COMMA)) {
            break;
        }
    }

    if (!parser_expect(parser, TOKEN_RPAREN, "Expected ')' after BASE type parameters")) {
        free(base_def);
        return NULL;
    }

    /* Validate required parameters */
    if (!base_def->input_function || !base_def->output_function) {
        parser_error(parser, "BASE type requires INPUT and OUTPUT functions");
        free(base_def);
        return NULL;
    }

    return base_def;
}
