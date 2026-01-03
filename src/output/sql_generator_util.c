#include "sql_generator.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Initialize default SQL generation options */
SQLGenOptions *sql_gen_options_default(void) {
    SQLGenOptions *opts = calloc(1, sizeof(SQLGenOptions));
    if (!opts) {
        return NULL;
    }

    opts->use_transactions = true;
    opts->use_if_exists = true;
    opts->add_comments = true;
    opts->add_warnings = true;
    opts->generate_rollback = false;
    opts->safe_mode = true;
    opts->schema_name = NULL;

    return opts;
}

void sql_gen_options_free(SQLGenOptions *opts) {
    free(opts);
}

/* Free SQL migration */
void sql_migration_free(SQLMigration *migration) {
    if (!migration) {
        return;
    }

    free(migration->forward_sql);
    free(migration->rollback_sql);
    free(migration);
}

/* Quote SQL identifier */
char *quote_identifier(const char *identifier) {
    if (!identifier) {
        return NULL;
    }

    /* Check if identifier needs quoting */
    bool needs_quote = false;

    /* Check if it contains special characters (except underscore) */
    for (const char *p = identifier; *p; p++) {
        if (!isalnum(*p) && *p != '_') {
            needs_quote = true;
            break;
        }
    }

    /* Check if it starts with a digit */
    if (!needs_quote && isdigit(identifier[0])) {
        needs_quote = true;
    }

    /* Don't quote if not necessary */
    if (!needs_quote) {
        return strdup(identifier);
    }

    /* Quote and escape */
    size_t len = strlen(identifier);
    char *quoted = malloc(len * 2 + 3);  /* Worst case: all quotes + surrounding quotes */
    if (!quoted) {
        return NULL;
    }

    char *dst = quoted;
    *dst++ = '"';

    for (const char *src = identifier; *src; src++) {
        if (*src == '"') {
            *dst++ = '"';
            *dst++ = '"';
        } else {
            *dst++ = *src;
        }
    }

    *dst++ = '"';
    *dst = '\0';

    return quoted;
}

/* Quote SQL literal */
char *quote_literal(const char *literal) {
    if (!literal) {
        return strdup("NULL");
    }

    size_t len = strlen(literal);
    char *quoted = malloc(len * 2 + 3);
    if (!quoted) {
        return NULL;
    }

    char *dst = quoted;
    *dst++ = '\'';

    for (const char *src = literal; *src; src++) {
        if (*src == '\'') {
            *dst++ = '\'';
            *dst++ = '\'';
        } else {
            *dst++ = *src;
        }
    }

    *dst++ = '\'';
    *dst = '\0';

    return quoted;
}

/* Format data type */
char *format_data_type(const char *type) {
    if (!type) {
        return NULL;
    }
    return strdup(type);
}
