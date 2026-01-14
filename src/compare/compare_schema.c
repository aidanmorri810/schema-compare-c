#include "compare.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

/* Compare two schemas */
SchemaDiff *compare_schemas(const Schema *source, const Schema *target,
                           const CompareOptions *opts,
                           MemoryContext *mem_ctx) {
    if (!source || !target) {
        return NULL;
    }

    SchemaDiff *result = schema_diff_create("public");
    if (!result) {
        return NULL;
    }

    /* Delegate to table comparison function */
    compare_all_tables(source->tables, source->table_count,
                         target->tables, target->table_count,
                         result, opts, mem_ctx);

    /* Future: Compare types, functions, procedures */

    return result;
}
