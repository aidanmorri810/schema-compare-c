#include "db_reader.h"
#include "utils.h"
#include <string.h>

/* Read complete schema from database (all object types) */
Schema *db_read_schema(DBConnection *conn, const char *schema_name,
                               MemoryContext *mem_ctx) {
    if (!conn || !db_is_connected(conn)) {
        return NULL;
    }

    if (!schema_name) {
        schema_name = "public";
    }

    log_info("Reading schema: %s", schema_name);

    /* Allocate schema structure */
    Schema *schema = mem_alloc(mem_ctx, sizeof(Schema));
    if (!schema) {
        log_error("Failed to allocate Schema");
        return NULL;
    }

    /* Initialize */
    schema->types = NULL;
    schema->type_count = 0;
    schema->tables = NULL;
    schema->table_count = 0;
    schema->functions = NULL;
    schema->function_count = 0;
    schema->procedures = NULL;
    schema->procedure_count = 0;

    /* Read tables */
    schema->tables = db_read_all_tables(conn, schema_name, &schema->table_count, mem_ctx);
    if (!schema->tables) {
        log_warn("No tables found in schema %s", schema_name);
    } else {
        log_info("Read %d tables from schema %s", schema->table_count, schema_name);
    }

    /* Future: Read types, functions, procedures */
    /* schema->types = db_read_types(conn, schema_name, &schema->type_count, mem_ctx); */
    /* schema->functions = db_read_functions(conn, schema_name, &schema->function_count, mem_ctx); */
    /* schema->procedures = db_read_procedures(conn, schema_name, &schema->procedure_count, mem_ctx); */

    log_info("Successfully read schema: %s", schema_name);
    return schema;
}
