#include "sql_generator.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

/* Column alteration operation types */
typedef enum {
    COL_ADD,
    COL_ALTER_TYPE,
    COL_ALTER_NULLABLE,
    COL_ALTER_DEFAULT,
    COL_DROP
} ColumnAlterOp;

/* Helper: Generate warning for column alteration */
static void append_alter_warning(StringBuilder *sb, ColumnAlterOp op, const SQLGenOptions *opts) {
    if (!opts->add_warnings) {
        return;
    }

    switch (op) {
        case COL_DROP:
            sb_append(sb, "-- WARNING: Dropping column - potential data loss\n");
            break;
        case COL_ALTER_TYPE:
            sb_append(sb, "-- WARNING: Changing column type may cause data conversion issues\n");
            break;
        default:
            break;
    }
}

/* Helper: Generate comment for column alteration */
static void append_alter_comment(StringBuilder *sb, ColumnAlterOp op, const ColumnDiff *col,
                                  const char *column_name, const SQLGenOptions *opts) {
    if (!opts->add_comments) {
        return;
    }

    switch (op) {
        case COL_ADD:
            sb_append(sb, "-- Add column ");
            sb_append(sb, column_name ? column_name : (col ? col->column_name : "unknown"));
            sb_append(sb, "\n");
            break;
        case COL_DROP:
            sb_append(sb, "-- Drop column ");
            sb_append(sb, column_name);
            sb_append(sb, "\n");
            break;
        case COL_ALTER_TYPE:
            sb_append_fmt(sb, "-- Change column type: %s → %s\n",
                         col->old_type ? col->old_type : "unknown",
                         col->new_type ? col->new_type : "unknown");
            break;
        case COL_ALTER_NULLABLE:
            sb_append_fmt(sb, "-- Change nullability: %s → %s\n",
                         col->old_nullable ? "NULL" : "NOT NULL",
                         col->new_nullable ? "NULL" : "NOT NULL");
            break;
        case COL_ALTER_DEFAULT:
            sb_append_fmt(sb, "-- Change default: %s → %s\n",
                         col->old_default ? col->old_default : "(none)",
                         col->new_default ? col->new_default : "(none)");
            break;
    }
}

/* Helper: Generate the ALTER/DROP/ADD clause for column alteration */
static void append_alter_clause(StringBuilder *sb, ColumnAlterOp op, const ColumnDiff *col,
                                 const char *quoted_column, const SQLGenOptions *opts) {
    switch (op) {
        case COL_ADD:
            sb_append(sb, " ADD COLUMN ");
            sb_append(sb, quoted_column);
            sb_append(sb, " ");
            sb_append(sb, col->new_type ? col->new_type : "text");
            /* Add DEFAULT clause if present */
            if (col->new_default) {
                sb_append(sb, " DEFAULT ");
                sb_append(sb, col->new_default);
            }
            /* Add NOT NULL constraint if column is not nullable */
            if (!col->new_nullable) {
                sb_append(sb, " NOT NULL");
            }
            break;
        case COL_DROP:
            sb_append(sb, " DROP COLUMN ");
            if (opts->use_if_exists) {
                sb_append(sb, "IF EXISTS ");
            }
            sb_append(sb, quoted_column);
            break;
        case COL_ALTER_TYPE:
            sb_append(sb, " ALTER COLUMN ");
            sb_append(sb, quoted_column);
            sb_append(sb, " TYPE ");
            sb_append(sb, col->new_type ? col->new_type : "text");
            break;
        case COL_ALTER_NULLABLE:
            sb_append(sb, " ALTER COLUMN ");
            sb_append(sb, quoted_column);
            if (col->new_nullable) {
                sb_append(sb, " DROP NOT NULL");
            } else {
                sb_append(sb, " SET NOT NULL");
            }
            break;
        case COL_ALTER_DEFAULT:
            sb_append(sb, " ALTER COLUMN ");
            sb_append(sb, quoted_column);
            if (col->new_default) {
                sb_append(sb, " SET DEFAULT ");
                sb_append(sb, col->new_default);
            } else {
                sb_append(sb, " DROP DEFAULT");
            }
            break;
    }
}

/* Internal: Generate ALTER TABLE column SQL
 * This is a generic function that handles all column operations.
 * Parameter validation should be done by the calling functions.
 */
static char *generate_alter_table_column_internal(const char *table_name, const ColumnDiff *col,
                                                    const char *column_name, ColumnAlterOp op,
                                                    const SQLGenOptions *opts) {
    StringBuilder *sb = sb_create();
    if (!sb) {
        return NULL;
    }

    /* Add warning if needed */
    append_alter_warning(sb, op, opts);

    /* Add comment if needed */
    append_alter_comment(sb, op, col, column_name ? column_name : (col ? col->column_name : NULL), opts);

    /* Quote identifiers */
    char *quoted_table = quote_identifier(table_name);
    char *quoted_column = quote_identifier(column_name ? column_name : col->column_name);

    /* Build ALTER TABLE statement */
    sb_append(sb, "ALTER TABLE ");
    sb_append(sb, quoted_table);

    append_alter_clause(sb, op, col, quoted_column, opts);

    sb_append(sb, ";\n");

    free(quoted_table);
    free(quoted_column);

    char *result = sb_to_string(sb);
    sb_free(sb);
    return result;
}

/* Generate ADD COLUMN SQL */
char *generate_add_column_sql(const char *table_name, const ColumnDiff *col,
                              const SQLGenOptions *opts) {
    if (!table_name || !col) {
        return NULL;
    }
    return generate_alter_table_column_internal(table_name, col, NULL, COL_ADD, opts);
}

/* Generate DROP COLUMN SQL */
char *generate_drop_column_sql(const char *table_name, const char *column_name,
                               const SQLGenOptions *opts) {
    if (!table_name || !column_name) {
        return NULL;
    }
    return generate_alter_table_column_internal(table_name, NULL, column_name, COL_DROP, opts);
}

/* Generate ALTER COLUMN TYPE SQL */
char *generate_alter_column_type_sql(const char *table_name, const ColumnDiff *col,
                                     const SQLGenOptions *opts) {
    if (!table_name || !col) {
        return NULL;
    }
    return generate_alter_table_column_internal(table_name, col, NULL, COL_ALTER_TYPE, opts);
}

/* Generate ALTER COLUMN SET/DROP NOT NULL SQL */
char *generate_alter_column_nullable_sql(const char *table_name, const ColumnDiff *col,
                                         const SQLGenOptions *opts) {
    if (!table_name || !col) {
        return NULL;
    }
    return generate_alter_table_column_internal(table_name, col, NULL, COL_ALTER_NULLABLE, opts);
}

/* Generate ALTER COLUMN SET/DROP DEFAULT SQL */
char *generate_alter_column_default_sql(const char *table_name, const ColumnDiff *col,
                                        const SQLGenOptions *opts) {
    if (!table_name || !col) {
        return NULL;
    }
    return generate_alter_table_column_internal(table_name, col, NULL, COL_ALTER_DEFAULT, opts);
}
