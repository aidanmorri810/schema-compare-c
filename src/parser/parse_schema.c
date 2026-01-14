#include "parser.h"
#include "pg_schema.h"
#include <string.h>

/* Parse a single CREATE statement and add it to the schema */
void parser_parse_statement(Parser *parser, Schema *schema) {
    if (!parser || !schema) {
        return;
    }

    if (!parser_check(parser, TOKEN_CREATE)) {
        parser_error(parser, "Expected CREATE statement");
        return;
    }

    parser_advance(parser); // Consume CREATE

    /* Peek at next token to determine statement type */
    if (parser_check(parser, TOKEN_TABLE) ||
        parser_check(parser, TOKEN_TEMPORARY) ||
        parser_check(parser, TOKEN_TEMP) ||
        parser_check(parser, TOKEN_UNLOGGED) ||
        parser_check(parser, TOKEN_GLOBAL) ||
        parser_check(parser, TOKEN_LOCAL)) {

        /* Back up to before CREATE (table parser expects to see CREATE) */
        parser->current = parser->previous;
        parser->previous.type = TOKEN_ERROR; // Mark as invalid

        CreateTableStmt *table = parser_parse_create_table(parser);
        if (!table) {
            return;
        }

        /* Resize tables array if needed */
        CreateTableStmt **new_tables = mem_realloc(parser->memory_ctx, schema->tables,
                                                    (schema->table_count + 1) * sizeof(CreateTableStmt *));
        if (!new_tables) {
            parser_error(parser, "Out of memory");
            return;
        }
        schema->tables = new_tables;
        schema->tables[schema->table_count++] = table;
        return;
    }

    /* Future: TOKEN_TYPE, TOKEN_INDEX, TOKEN_FUNCTION, etc. */

    parser_error(parser, "Unknown CREATE statement type");
}

Schema *parse_all_statements(Parser *parser) {
    if (!parser) {
        return NULL;
    }

    /* Allocate schema structure */
    Schema *schema = mem_alloc(parser->memory_ctx, sizeof(Schema));
    if (!schema) {
        parser_error(parser, "Out of memory");
        return NULL;
    }

    /* Initialize schema */
    schema->types = NULL;
    schema->type_count = 0;
    schema->tables = NULL;
    schema->table_count = 0;
    schema->functions = NULL;
    schema->function_count = 0;
    schema->procedures = NULL;
    schema->procedure_count = 0;

    /* Parse statements until EOF */
    while (!parser_check(parser, TOKEN_EOF)) {
        /* Skip any leading semicolons */
        while (parser_match(parser, TOKEN_SEMICOLON)) {
            /* Skip empty statements */
        }

        /* Check for EOF after skipping semicolons */
        if (parser_check(parser, TOKEN_EOF)) {
            break;
        }

        /* Parse next statement and add to schema */
        parser_parse_statement(parser, schema);

        /* Handle errors by synchronizing and continuing */
        if (parser->had_error) {
            parser_synchronize(parser);
            parser->panic_mode = false;
        }

        /* Expect semicolon after statement (but don't error on EOF) */
        if (!parser_check(parser, TOKEN_EOF) && !parser_check(parser, TOKEN_SEMICOLON)) {
            parser_error(parser, "Expected semicolon after statement");
            parser_synchronize(parser);
        }
    }

    return schema;
}