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

/* Check if identifier is a PostgreSQL reserved keyword */
static bool is_reserved_keyword(const char *identifier) {
    /* Common PostgreSQL reserved keywords that need quoting */
    static const char *keywords[] = {
        "select", "from", "where", "insert", "update", "delete",
        "create", "drop", "alter", "table", "index", "view",
        "user", "group", "order", "by", "limit", "offset",
        "union", "join", "left", "right", "inner", "outer",
        "on", "as", "and", "or", "not", "null", "true", "false",
        "default", "primary", "foreign", "key", "references",
        "constraint", "unique", "check", "exists", "like",
        NULL
    };

    char lower[256];
    size_t len = strlen(identifier);
    if (len >= sizeof(lower)) {
        return false;
    }

    /* Convert to lowercase for comparison */
    for (size_t i = 0; i < len; i++) {
        lower[i] = tolower(identifier[i]);
    }
    lower[len] = '\0';

    for (int i = 0; keywords[i] != NULL; i++) {
        if (strcmp(lower, keywords[i]) == 0) {
            return true;
        }
    }

    return false;
}

/* Append quoted SQL identifier to string builder */
void sb_append_identifier(StringBuilder *sb, const char *identifier) {
    if (!sb || !identifier) {
        return;
    }

    /* Check if identifier needs quoting */
    bool needs_quote = false;

    /* Check if it's a reserved keyword */
    if (is_reserved_keyword(identifier)) {
        needs_quote = true;
    }

    /* Check if it contains special characters (except underscore) */
    if (!needs_quote) {
        for (const char *p = identifier; *p; p++) {
            if (!isalnum(*p) && *p != '_') {
                needs_quote = true;
                break;
            }
        }
    }

    /* Check if it starts with a digit */
    if (!needs_quote && isdigit(identifier[0])) {
        needs_quote = true;
    }

    /* Don't quote if not necessary */
    if (!needs_quote) {
        sb_append(sb, identifier);
        return;
    }

    /* Quote and escape */
    sb_append(sb, "\"");

    for (const char *src = identifier; *src; src++) {
        if (*src == '"') {
            sb_append(sb, "\"\"");
        } else {
            sb_append_char(sb, *src);
        }
    }

    sb_append(sb, "\"");
}

/* Append quoted SQL literal to string builder */
void sb_append_literal(StringBuilder *sb, const char *literal) {
    if (!sb) {
        return;
    }

    if (!literal) {
        sb_append(sb, "NULL");
        return;
    }

    sb_append(sb, "'");

    for (const char *src = literal; *src; src++) {
        if (*src == '\'') {
            sb_append(sb, "''");
        } else {
            sb_append_char(sb, *src);
        }
    }

    sb_append(sb, "'");
}

/* Format data type */
char *format_data_type(const char *type) {
    if (!type) {
        return NULL;
    }
    return strdup(type);
}
