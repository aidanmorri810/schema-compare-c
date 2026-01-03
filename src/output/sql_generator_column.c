#include "sql_generator.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

/* Generate ADD COLUMN SQL */
char *generate_add_column_sql(const char *table_name, const ColumnDiff *col,
                              const SQLGenOptions *opts) {
    if (!table_name || !col) {
        return NULL;
    }

    StringBuilder *sb = sb_create();
    if (!sb) {
        return NULL;
    }

    if (opts->add_comments) {
        sb_append(sb, "-- Add column ");
        sb_append(sb, col->column_name);
        sb_append(sb, "\n");
    }

    char *quoted_table = quote_identifier(table_name);
    char *quoted_column = quote_identifier(col->column_name);

    sb_append(sb, "ALTER TABLE ");
    sb_append(sb, quoted_table);
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

    sb_append(sb, ";\n");

    free(quoted_table);
    free(quoted_column);

    char *result = sb_to_string(sb);
    sb_free(sb);
    return result;
}

/* Generate DROP COLUMN SQL */
char *generate_drop_column_sql(const char *table_name, const char *column_name,
                               const SQLGenOptions *opts) {
    if (!table_name || !column_name) {
        return NULL;
    }

    StringBuilder *sb = sb_create();
    if (!sb) {
        return NULL;
    }

    if (opts->add_warnings) {
        sb_append(sb, "-- WARNING: Dropping column - potential data loss\n");
    }

    if (opts->add_comments) {
        sb_append(sb, "-- Drop column ");
        sb_append(sb, column_name);
        sb_append(sb, "\n");
    }

    char *quoted_table = quote_identifier(table_name);
    char *quoted_column = quote_identifier(column_name);

    sb_append(sb, "ALTER TABLE ");
    sb_append(sb, quoted_table);
    sb_append(sb, " DROP COLUMN ");
    if (opts->use_if_exists) {
        sb_append(sb, "IF EXISTS ");
    }
    sb_append(sb, quoted_column);
    sb_append(sb, ";\n");

    free(quoted_table);
    free(quoted_column);

    char *result = sb_to_string(sb);
    sb_free(sb);
    return result;
}

/* Generate ALTER COLUMN TYPE SQL */
char *generate_alter_column_type_sql(const char *table_name, const ColumnDiff *col,
                                     const SQLGenOptions *opts) {
    if (!table_name || !col) {
        return NULL;
    }

    StringBuilder *sb = sb_create();
    if (!sb) {
        return NULL;
    }

    if (opts->add_warnings) {
        sb_append(sb, "-- WARNING: Changing column type may cause data conversion issues\n");
    }

    if (opts->add_comments) {
        sb_append_fmt(sb, "-- Change column type: %s → %s\n",
                     col->old_type ? col->old_type : "unknown",
                     col->new_type ? col->new_type : "unknown");
    }

    char *quoted_table = quote_identifier(table_name);
    char *quoted_column = quote_identifier(col->column_name);

    sb_append(sb, "ALTER TABLE ");
    sb_append(sb, quoted_table);
    sb_append(sb, " ALTER COLUMN ");
    sb_append(sb, quoted_column);
    sb_append(sb, " TYPE ");
    sb_append(sb, col->new_type ? col->new_type : "text");
    sb_append(sb, ";\n");

    free(quoted_table);
    free(quoted_column);

    char *result = sb_to_string(sb);
    sb_free(sb);
    return result;
}

/* Generate ALTER COLUMN SET/DROP NOT NULL SQL */
char *generate_alter_column_nullable_sql(const char *table_name, const ColumnDiff *col,
                                         const SQLGenOptions *opts) {
    if (!table_name || !col) {
        return NULL;
    }

    StringBuilder *sb = sb_create();
    if (!sb) {
        return NULL;
    }

    if (opts->add_comments) {
        sb_append_fmt(sb, "-- Change nullability: %s → %s\n",
                     col->old_nullable ? "NULL" : "NOT NULL",
                     col->new_nullable ? "NULL" : "NOT NULL");
    }

    char *quoted_table = quote_identifier(table_name);
    char *quoted_column = quote_identifier(col->column_name);

    sb_append(sb, "ALTER TABLE ");
    sb_append(sb, quoted_table);
    sb_append(sb, " ALTER COLUMN ");
    sb_append(sb, quoted_column);

    if (col->new_nullable) {
        sb_append(sb, " DROP NOT NULL");
    } else {
        sb_append(sb, " SET NOT NULL");
    }

    sb_append(sb, ";\n");

    free(quoted_table);
    free(quoted_column);

    char *result = sb_to_string(sb);
    sb_free(sb);
    return result;
}

/* Generate ALTER COLUMN SET/DROP DEFAULT SQL */
char *generate_alter_column_default_sql(const char *table_name, const ColumnDiff *col,
                                        const SQLGenOptions *opts) {
    if (!table_name || !col) {
        return NULL;
    }

    StringBuilder *sb = sb_create();
    if (!sb) {
        return NULL;
    }

    if (opts->add_comments) {
        sb_append_fmt(sb, "-- Change default: %s → %s\n",
                     col->old_default ? col->old_default : "(none)",
                     col->new_default ? col->new_default : "(none)");
    }

    char *quoted_table = quote_identifier(table_name);
    char *quoted_column = quote_identifier(col->column_name);

    sb_append(sb, "ALTER TABLE ");
    sb_append(sb, quoted_table);
    sb_append(sb, " ALTER COLUMN ");
    sb_append(sb, quoted_column);

    if (col->new_default) {
        sb_append(sb, " SET DEFAULT ");
        sb_append(sb, col->new_default);
    } else {
        sb_append(sb, " DROP DEFAULT");
    }

    sb_append(sb, ";\n");

    free(quoted_table);
    free(quoted_column);

    char *result = sb_to_string(sb);
    sb_free(sb);
    return result;
}
