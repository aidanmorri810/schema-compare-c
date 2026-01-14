#include "compare.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdio.h>

/* Initialize default comparison options */
CompareOptions *compare_options_default(void) {
    CompareOptions *opts = calloc(1, sizeof(CompareOptions));
    if (!opts) {
        return NULL;
    }

    opts->compare_tablespaces = false;
    opts->compare_storage_params = true;
    opts->compare_constraints = true;
    opts->compare_partitioning = true;
    opts->compare_inheritance = true;

    opts->case_sensitive = false;
    opts->normalize_types = true;
    opts->ignore_constraint_names = true;  /* Ignore constraint names to handle DB-generated vs user-defined names */
    opts->ignore_whitespace = true;

    opts->exclude_tables = NULL;
    opts->include_tables = NULL;
    opts->exclude_count = 0;
    opts->include_count = 0;

    return opts;
}

void compare_options_free(CompareOptions *opts) {
    if (!opts) {
        return;
    }

    for (int i = 0; i < opts->exclude_count; i++) {
        free(opts->exclude_tables[i]);
    }
    free(opts->exclude_tables);

    for (int i = 0; i < opts->include_count; i++) {
        free(opts->include_tables[i]);
    }
    free(opts->include_tables);

    free(opts);
}

/* Check if table should be included in comparison */
bool should_compare_table(const char *table_name, const CompareOptions *opts) {
    if (!opts) {
        return true;
    }

    /* If include list is specified, table must match at least one pattern */
    if (opts->include_count > 0) {
        bool matched = false;
        for (int i = 0; i < opts->include_count; i++) {
            /* Simple wildcard matching - just prefix matching for now */
            if (strstr(table_name, opts->include_tables[i]) != NULL) {
                matched = true;
                break;
            }
        }
        if (!matched) {
            return false;
        }
    }

    /* Check exclude list */
    for (int i = 0; i < opts->exclude_count; i++) {
        if (strstr(table_name, opts->exclude_tables[i]) != NULL) {
            return false;
        }
    }

    return true;
}

/* Normalize type name for comparison */
char *normalize_type_name(const char *type_name, MemoryContext *mem_ctx) {
    if (!type_name) {
        return NULL;
    }

    /* Common PostgreSQL type aliases */
    static const struct {
        const char *alias;
        const char *canonical;
    } type_map[] = {
        {"int2", "smallint"},
        {"int4", "integer"},
        {"int8", "bigint"},
        {"float4", "real"},
        {"float8", "double precision"},
        {"bool", "boolean"},
        {"varchar", "character varying"},
        {"char", "character"},
        {NULL, NULL}
    };

    /* Convert to lowercase */
    char *normalized = mem_strdup(mem_ctx, type_name);
    if (!normalized) {
        return NULL;
    }

    for (char *p = normalized; *p; p++) {
        *p = tolower(*p);
    }

    /* Remove schema qualification (e.g., "public.enum_type" -> "enum_type") */
    /* This handles cases where database returns "public.review_status" but file has "review_status" */
    char *dot = strchr(normalized, '.');
    if (dot) {
        /* Check if it's a schema.type pattern (not numeric like "3.14") */
        bool is_schema = true;
        for (char *p = normalized; p < dot; p++) {
            if (!isalpha(*p) && *p != '_') {
                is_schema = false;
                break;
            }
        }
        if (is_schema) {
            /* Move everything after the dot to the beginning */
            memmove(normalized, dot + 1, strlen(dot + 1) + 1);
        }
    }

    /* Normalize timestamp variations */
    /* "timestamp(3) without time zone" -> "timestamp(3)" */
    char *without_tz = strstr(normalized, " without time zone");
    if (without_tz) {
        *without_tz = '\0';
    }
    /* "timestamp without time zone" -> "timestamp" */
    /* This is already handled by the above */

    /* "timestamp(3) with time zone" -> "timestamptz(3)" for consistency */
    char *with_tz = strstr(normalized, " with time zone");
    if (with_tz) {
        /* Find if there's a precision specifier */
        char *open_paren = strchr(normalized, '(');
        if (open_paren && open_paren < with_tz) {
            /* timestamp(3) with time zone -> timestamptz(3) */
            char *close_paren = strchr(open_paren, ')');
            if (close_paren && close_paren < with_tz) {
                char precision[32];
                size_t prec_len = close_paren - open_paren + 1;
                if (prec_len < sizeof(precision)) {
                    memcpy(precision, open_paren, prec_len);
                    precision[prec_len] = '\0';
                    sprintf(normalized, "timestamptz%s", precision);
                }
            }
        } else {
            /* timestamp with time zone -> timestamptz */
            strcpy(normalized, "timestamptz");
        }
    }

    /* Check for type aliases */
    for (int i = 0; type_map[i].alias; i++) {
        if (strcmp(normalized, type_map[i].alias) == 0) {
            char *canonical = mem_strdup(mem_ctx, type_map[i].canonical);
            return canonical;
        }
    }

    return normalized;
}

/* Compare data types */
bool data_types_equal(const char *type1, const char *type2, const CompareOptions *opts) {
    if (type1 == type2) {
        return true;
    }
    if (!type1 || !type2) {
        return false;
    }

    if (opts && opts->normalize_types) {
        /* Need temporary memory context for normalization */
        MemoryContext *temp_ctx = memory_context_create("temp_type_compare");
        char *norm1 = normalize_type_name(type1, temp_ctx);
        char *norm2 = normalize_type_name(type2, temp_ctx);
        bool equal = (strcmp(norm1, norm2) == 0);
        memory_context_destroy(temp_ctx);
        return equal;
    }

    return strcmp(type1, type2) == 0;
}

/* Helper to normalize default expressions for comparison */
static char *normalize_expression(const char *expr) {
    if (!expr) {
        return NULL;
    }

    /* Allocate buffer for normalized expression */
    char *normalized = strdup(expr);
    if (!normalized) {
        return NULL;
    }

    /* Remove type casts (e.g., 'DRAFT'::review_status -> 'DRAFT') */
    char *cast = strstr(normalized, "::");
    if (cast) {
        /* Find the end of the value before the cast */
        /* We need to handle quoted strings and unquoted values */
        *cast = '\0';
    }

    return normalized;
}

/* Compare expressions accounting for whitespace */
bool expressions_equal(const char *expr1, const char *expr2, const CompareOptions *opts) {
    if (expr1 == expr2) {
        return true;
    }
    if (!expr1 || !expr2) {
        return false;
    }

    /* Normalize expressions to remove type casts */
    char *norm1 = normalize_expression(expr1);
    char *norm2 = normalize_expression(expr2);

    bool equal = false;

    if (opts && opts->ignore_whitespace) {
        /* Remove all whitespace and compare */
        char *no_ws1 = str_remove_whitespace(norm1);
        char *no_ws2 = str_remove_whitespace(norm2);
        equal = (strcmp(no_ws1, no_ws2) == 0);
        free(no_ws1);
        free(no_ws2);
    } else {
        equal = (strcmp(norm1, norm2) == 0);
    }

    free(norm1);
    free(norm2);

    return equal;
}

/* Case-sensitive or insensitive string comparison */
bool names_equal(const char *name1, const char *name2, const CompareOptions *opts) {
    if (name1 == name2) {
        return true;
    }
    if (!name1 || !name2) {
        return false;
    }

    if (opts && !opts->case_sensitive) {
        return strcasecmp(name1, name2) == 0;
    }

    return strcmp(name1, name2) == 0;
}
