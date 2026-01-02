#include "schema_compare.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>

/* Print version */
void print_version(void) {
    printf("schema-compare version %s\n", SCHEMA_COMPARE_VERSION);
    printf("PostgreSQL schema comparison tool\n");
}

/* Print usage */
void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS] SOURCE TARGET\n\n", program_name);
    printf("Compare PostgreSQL schemas and generate migration scripts.\n\n");
    printf("Arguments:\n");
    printf("  SOURCE          Source schema (database connection string or DDL file/directory)\n");
    printf("  TARGET          Target schema (database connection string or DDL file/directory)\n\n");
    printf("Options:\n");
    printf("  -o, --output FILE        Write report to FILE (default: stdout)\n");
    printf("  -s, --sql [FILE]         Generate SQL migration script (to FILE or stdout)\n");
    printf("  -f, --format FORMAT      Report format: text, markdown (default: text)\n");
    printf("  -v, --verbose            Verbose output\n");
    printf("  -q, --quiet              Quiet mode (errors only)\n");
    printf("  --no-color               Disable colored output\n");
    printf("  --no-transactions        Don't wrap SQL in transactions\n");
    printf("  --schema NAME            Schema name for database sources (default: public)\n");
    printf("  -h, --help               Show this help message\n");
    printf("  -V, --version            Show version information\n\n");
    printf("Database Connection Strings:\n");
    printf("  Use PostgreSQL connection string format:\n");
    printf("    postgresql://user:password@host:port/database\n");
    printf("    host=localhost port=5432 dbname=mydb user=myuser\n\n");
    printf("Examples:\n");
    printf("  Compare two databases:\n");
    printf("    %s 'host=localhost dbname=prod' 'host=localhost dbname=dev'\n\n", program_name);
    printf("  Compare database with DDL file:\n");
    printf("    %s 'host=localhost dbname=mydb' schema.sql\n\n", program_name);
    printf("  Generate migration SQL to file:\n");
    printf("    %s --sql=migration.sql schema_v1.sql schema_v2.sql\n\n", program_name);
    printf("  Generate migration SQL to stdout:\n");
    printf("    %s --sql schema_v1.sql schema_v2.sql\n\n", program_name);
    printf("  Migrate from directory to database:\n");
    printf("    %s --sql 'host=localhost dbname=mydb' tests/data/nora/src/nora-tenant\n\n", program_name);
}

/* Check if path is a directory */
static bool is_directory(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return false;
    }
    return S_ISDIR(st.st_mode);
}

/* Check if path is a file */
static bool is_file(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return false;
    }
    return S_ISREG(st.st_mode);
}

/* Check if string looks like a database connection string */
static bool is_database_connection(const char *str) {
    /* Check for postgresql:// URI format */
    if (strncmp(str, "postgresql://", 13) == 0 || strncmp(str, "postgres://", 11) == 0) {
        return true;
    }

    /* Check for key=value format (host=, dbname=, etc.) */
    if (strstr(str, "host=") || strstr(str, "dbname=") || strstr(str, "port=")) {
        return true;
    }

    return false;
}

/* Parse database connection string into DBConfig */
static DBConfig parse_db_connection_string(const char *connstr) {
    DBConfig config = {0};

    /* Set defaults */
    config.host = strdup("localhost");
    config.port = strdup("5432");
    config.database = NULL;
    config.user = NULL;
    config.password = NULL;
    config.connect_timeout = 30;

    /* Simple parser for key=value format */
    char *str_copy = strdup(connstr);
    char *saveptr = NULL;
    char *token = strtok_r(str_copy, " ", &saveptr);

    while (token) {
        char *equals = strchr(token, '=');
        if (equals) {
            *equals = '\0';
            char *key = token;
            char *value = equals + 1;

            if (strcmp(key, "host") == 0) {
                free(config.host);
                config.host = strdup(value);
            } else if (strcmp(key, "port") == 0) {
                free(config.port);
                config.port = strdup(value);
            } else if (strcmp(key, "dbname") == 0 || strcmp(key, "database") == 0) {
                free(config.database);
                config.database = strdup(value);
            } else if (strcmp(key, "user") == 0) {
                free(config.user);
                config.user = strdup(value);
            } else if (strcmp(key, "password") == 0) {
                free(config.password);
                config.password = strdup(value);
            }
        }
        token = strtok_r(NULL, " ", &saveptr);
    }

    free(str_copy);
    return config;
}

/* Extract database and schema from directory path
 * Expected structure: src/[database]/[schema]/table/...
 * Returns true if extraction successful
 */
static bool extract_db_schema_from_path(const char *dir_path, char **database, char **schema) {
    *database = NULL;
    *schema = NULL;

    /* Look for pattern: .../[database]/[schema]/... */
    char *path_copy = strdup(dir_path);
    if (!path_copy) return false;

    /* Remove trailing slashes */
    size_t len = strlen(path_copy);
    while (len > 0 && path_copy[len - 1] == '/') {
        path_copy[--len] = '\0';
    }

    /* Split by '/' and look for the structure */
    char *parts[32];
    int count = 0;
    char *saveptr = NULL;
    char *token = strtok_r(path_copy, "/", &saveptr);

    while (token && count < 32) {
        parts[count++] = token;
        token = strtok_r(NULL, "/", &saveptr);
    }

    /* Look for "src" directory, then extract database/schema after it
     * Common patterns:
     * - tests/data/nora/src/nora-tenant/public/table
     * - src/nora-tenant/public
     * After "src", next two parts should be: [database]/[schema]
     */
    int src_index = -1;
    for (int i = 0; i < count; i++) {
        if (strcmp(parts[i], "src") == 0) {
            src_index = i;
            break;
        }
    }

    if (src_index >= 0 && src_index + 2 <= count) {
        /* Found src/[database]/[schema] pattern */
        *database = strdup(parts[src_index + 1]);
        if (src_index + 2 < count) {
            *schema = strdup(parts[src_index + 2]);
        } else {
            *schema = strdup("public");  /* Default schema */
        }
    } else if (count >= 3 && strcmp(parts[count - 1], "table") == 0) {
        /* Fallback: Pattern .../[database]/[schema]/table */
        *database = strdup(parts[count - 3]);
        *schema = strdup(parts[count - 2]);
    } else if (count >= 2) {
        /* Fallback: Pattern .../[database]/[schema] */
        *database = strdup(parts[count - 2]);
        *schema = strdup(parts[count - 1]);
    }

    free(path_copy);
    return (*database != NULL && *schema != NULL);
}

/* Parse a schema source specification */
SchemaSource *parse_schema_source(const char *spec) {
    if (!spec) {
        return NULL;
    }

    SchemaSource *source = calloc(1, sizeof(SchemaSource));
    if (!source) {
        return NULL;
    }

    /* Determine source type */
    if (is_database_connection(spec)) {
        source->type = SOURCE_TYPE_DATABASE;
        source->source.db_config = parse_db_connection_string(spec);

        /* Validate required fields */
        if (!source->source.db_config.database) {
            fprintf(stderr, "Error: Database name is required in connection string\n");
            free(source->source.db_config.host);
            free(source->source.db_config.port);
            free(source);
            return NULL;
        }

        /* Set database name from connection config */
        source->database_name = strdup(source->source.db_config.database);
    } else if (is_directory(spec)) {
        source->type = SOURCE_TYPE_DIRECTORY;
        source->source.directory_path = strdup(spec);
        if (!source->source.directory_path) {
            free(source);
            return NULL;
        }

        /* Try to extract database and schema from directory structure */
        char *db_name = NULL, *schema_name = NULL;
        if (extract_db_schema_from_path(spec, &db_name, &schema_name)) {
            source->database_name = db_name;
            source->schema_name = schema_name;
            log_info("Extracted from directory: database='%s', schema='%s'",
                    db_name, schema_name);
        }
    } else if (is_file(spec)) {
        source->type = SOURCE_TYPE_FILE;
        source->source.file_path = strdup(spec);
        if (!source->source.file_path) {
            free(source);
            return NULL;
        }
    } else {
        fprintf(stderr, "Invalid source specification: %s\n", spec);
        free(source);
        return NULL;
    }

    return source;
}

/* Free schema source */
void schema_source_free(SchemaSource *source) {
    if (!source) {
        return;
    }

    switch (source->type) {
        case SOURCE_TYPE_FILE:
            free(source->source.file_path);
            break;
        case SOURCE_TYPE_DIRECTORY:
            free(source->source.directory_path);
            break;
        case SOURCE_TYPE_DATABASE:
            free(source->source.db_config.host);
            free(source->source.db_config.port);
            free(source->source.db_config.database);
            free(source->source.db_config.user);
            free(source->source.db_config.password);
            break;
    }

    free(source->database_name);
    free(source->schema_name);
    free(source);
}

/* Create application context */
AppContext *app_context_create(void) {
    AppContext *ctx = calloc(1, sizeof(AppContext));
    if (!ctx) {
        return NULL;
    }

    ctx->compare_opts = compare_options_default();
    ctx->report_opts = report_options_default();
    ctx->sql_opts = sql_gen_options_default();

    ctx->generate_sql = false;
    ctx->generate_report = true;
    ctx->verbose = false;
    ctx->quiet = false;

    return ctx;
}

/* Free application context */
void app_context_free(AppContext *ctx) {
    if (!ctx) {
        return;
    }

    compare_options_free(ctx->compare_opts);
    report_options_free(ctx->report_opts);
    sql_gen_options_free(ctx->sql_opts);

    schema_source_free(ctx->source);
    schema_source_free(ctx->target);

    free(ctx);
}

/* Parse command line arguments */
AppContext *parse_command_line(int argc, char **argv) {
    AppContext *ctx = app_context_create();
    if (!ctx) {
        return NULL;
    }

    static struct option long_options[] = {
        {"output",          required_argument, 0, 'o'},
        {"sql",             optional_argument, 0, 's'},
        {"format",          required_argument, 0, 'f'},
        {"verbose",         no_argument,       0, 'v'},
        {"quiet",           no_argument,       0, 'q'},
        {"no-color",        no_argument,       0, 'C'},
        {"no-transactions", no_argument,       0, 'T'},
        {"schema",          required_argument, 0, 'S'},
        {"help",            no_argument,       0, 'h'},
        {"version",         no_argument,       0, 'V'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "o:s::f:vqhV", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'o':
                ctx->report_output_file = optarg;
                break;
            case 's':
                ctx->generate_sql = true;
                if (optarg) {
                    ctx->sql_output_file = optarg;
                } else {
                    /* SQL goes to stdout, suppress report unless explicitly requested */
                    ctx->generate_report = false;
                }
                break;
            case 'f':
                if (strcmp(optarg, "markdown") == 0) {
                    ctx->report_opts->format = REPORT_FORMAT_MARKDOWN;
                }
                break;
            case 'v':
                ctx->verbose = true;
                ctx->report_opts->verbosity = REPORT_VERBOSITY_DETAILED;
                break;
            case 'q':
                ctx->quiet = true;
                ctx->generate_report = false;
                break;
            case 'C':
                ctx->report_opts->use_color = false;
                break;
            case 'T':
                ctx->sql_opts->use_transactions = false;
                break;
            case 'h':
                print_usage(argv[0]);
                app_context_free(ctx);
                exit(0);
            case 'V':
                print_version();
                app_context_free(ctx);
                exit(0);
            default:
                print_usage(argv[0]);
                app_context_free(ctx);
                return NULL;
        }
    }

    /* Check for source and target arguments */
    if (optind + 2 > argc) {
        fprintf(stderr, "Error: SOURCE and TARGET arguments are required\n\n");
        print_usage(argv[0]);
        app_context_free(ctx);
        return NULL;
    }

    /* Parse source and target specifications */
    ctx->source = parse_schema_source(argv[optind]);
    ctx->target = parse_schema_source(argv[optind + 1]);

    if (!ctx->source || !ctx->target) {
        fprintf(stderr, "Error: Failed to parse source or target specification\n");
        app_context_free(ctx);
        return NULL;
    }

    return ctx;
}

/* Load schemas from database */
CreateTableStmt **load_from_database(DBConnection *conn, const char *schema,
                                     int *table_count, MemoryContext *mem_ctx) {
    IntrospectionOptions opts = {0};
    opts.schemas = (char **)&schema;
    opts.schema_count = 1;

    return db_read_all_tables(conn, &opts, table_count, mem_ctx);
}

/* Load schemas from file
 *
 * Returns array of CreateTableStmt* that caller must free with:
 *   for (int i = 0; i < count; i++) free_create_table_stmt(stmts[i]);
 *   free(stmts);
 */
CreateTableStmt **load_from_file(const char *file_path, int *table_count,
                                 MemoryContext *mem_ctx) {
    (void)mem_ctx;  /* TODO */

    /* Read file content */
    char *source = read_file_to_string(file_path);
    if (!source) {
        *table_count = 0;
        return NULL;
    }

    Parser *parser = parser_create(source);
    if (!parser) {
        free(source);
        *table_count = 0;
        return NULL;
    }

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    parser_destroy(parser);
    free(source);

    if (!stmt) {
        *table_count = 0;
        return NULL;
    }

    CreateTableStmt **result = malloc(sizeof(CreateTableStmt *));
    if (!result) {
        free_create_table_stmt(stmt);
        *table_count = 0;
        return NULL;
    }

    result[0] = stmt;
    *table_count = 1;
    return result;
}

/* Recursively find all .sql files in directory */
static char **find_sql_files_recursive(const char *dir_path, int *count) {
    *count = 0;
    int capacity = 64;
    char **files = malloc(sizeof(char *) * capacity);
    if (!files) {
        return NULL;
    }

    /* Use a simple stack for directory traversal */
    char **dir_stack = malloc(sizeof(char *) * 256);
    if (!dir_stack) {
        free(files);
        return NULL;
    }

    int stack_size = 0;
    dir_stack[stack_size++] = strdup(dir_path);

    while (stack_size > 0) {
        char *current_dir = dir_stack[--stack_size];
        DIR *dir = opendir(current_dir);

        if (!dir) {
            free(current_dir);
            continue;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            char path[PATH_MAX];
            snprintf(path, sizeof(path), "%s/%s", current_dir, entry->d_name);

            struct stat st;
            if (stat(path, &st) != 0) {
                continue;
            }

            if (S_ISDIR(st.st_mode)) {
                /* Add directory to stack */
                if (stack_size < 256) {
                    dir_stack[stack_size++] = strdup(path);
                }
            } else if (S_ISREG(st.st_mode)) {
                /* Check if it's a .sql file */
                size_t name_len = strlen(entry->d_name);
                if (name_len > 4 && strcmp(entry->d_name + name_len - 4, ".sql") == 0) {
                    /* Expand array if needed */
                    if (*count >= capacity) {
                        capacity *= 2;
                        char **new_files = realloc(files, sizeof(char *) * capacity);
                        if (!new_files) {
                            break;
                        }
                        files = new_files;
                    }

                    files[*count] = strdup(path);
                    if (files[*count]) {
                        (*count)++;
                    }
                }
            }
        }

        closedir(dir);
        free(current_dir);
    }

    /* Clean up directory stack */
    for (int i = 0; i < stack_size; i++) {
        free(dir_stack[i]);
    }
    free(dir_stack);

    return files;
}

/* Load schemas from directory
 * Directory structure: src/[database]/[schema]/table/[table_name].sql
 */
CreateTableStmt **load_from_directory(const char *dir_path, int *table_count,
                                      MemoryContext *mem_ctx) {
    (void)mem_ctx;  /* TODO: Use memory context */

    /* Find all .sql files in directory tree */
    int file_count = 0;
    char **sql_files = find_sql_files_recursive(dir_path, &file_count);

    if (!sql_files || file_count == 0) {
        *table_count = 0;
        if (sql_files) {
            free(sql_files);
        }
        return NULL;
    }

    /* Allocate array for table statements */
    CreateTableStmt **tables = malloc(sizeof(CreateTableStmt *) * file_count);
    if (!tables) {
        for (int i = 0; i < file_count; i++) {
            free(sql_files[i]);
        }
        free(sql_files);
        *table_count = 0;
        return NULL;
    }

    int parsed_count = 0;

    /* Parse each SQL file */
    for (int i = 0; i < file_count; i++) {
        char *source = read_file_to_string(sql_files[i]);
        if (!source) {
            log_warn("Failed to read file: %s", sql_files[i]);
            free(sql_files[i]);
            continue;
        }

        Parser *parser = parser_create(source);
        if (!parser) {
            log_warn("Failed to create parser for: %s", sql_files[i]);
            free(source);
            free(sql_files[i]);
            continue;
        }

        CreateTableStmt *stmt = parser_parse_create_table(parser);
        parser_destroy(parser);
        free(source);

        if (!stmt) {
            log_warn("Failed to parse CREATE TABLE from: %s", sql_files[i]);
            free(sql_files[i]);
            continue;
        }

        tables[parsed_count++] = stmt;
        free(sql_files[i]);
    }

    free(sql_files);

    if (parsed_count == 0) {
        free(tables);
        *table_count = 0;
        return NULL;
    }

    /* Resize array to actual count if needed */
    if (parsed_count < file_count) {
        CreateTableStmt **resized = realloc(tables, sizeof(CreateTableStmt *) * parsed_count);
        if (resized) {
            tables = resized;
        }
    }

    *table_count = parsed_count;
    return tables;
}

/* Main comparison workflow */
int compare_and_report(SchemaSource *source, SchemaSource *target, AppContext *ctx) {
    (void)source;
    (void)target;
    (void)ctx;

    /* TODO: Implement full workflow */
    printf("Comparison workflow not yet implemented\n");
    return 1;
}

/* Main entry point */
int main(int argc, char **argv) {
    /* Initialize logging */
    log_init(NULL, LOG_LEVEL_INFO);

    /* Parse command line */
    AppContext *ctx = parse_command_line(argc, argv);
    if (!ctx) {
        log_shutdown();
        return 1;
    }

    /* Load schemas based on source type */
    int source_count = 0, target_count = 0;
    CreateTableStmt **source_tables = NULL;
    CreateTableStmt **target_tables = NULL;
    DBConnection *source_conn = NULL;
    DBConnection *target_conn = NULL;

    /* Load source */
    if (ctx->source->type == SOURCE_TYPE_DATABASE) {
        log_info("Connecting to source database: %s@%s:%s/%s",
                ctx->source->source.db_config.user ? ctx->source->source.db_config.user : "default",
                ctx->source->source.db_config.host,
                ctx->source->source.db_config.port,
                ctx->source->source.db_config.database);

        source_conn = db_connect(&ctx->source->source.db_config);
        if (!source_conn || !db_is_connected(source_conn)) {
            log_error("Failed to connect to source database: %s",
                     source_conn ? db_get_error(source_conn) : "connection failed");
        } else {
            const char *schema = ctx->source->schema_name ? ctx->source->schema_name : "public";
            source_tables = load_from_database(source_conn, schema, &source_count, NULL);
            if (source_tables) {
                log_info("Loaded %d tables from source database", source_count);
            } else {
                log_error("Failed to load tables from source database");
            }
        }
    } else if (ctx->source->type == SOURCE_TYPE_DIRECTORY) {
        log_info("Loading source from directory: %s", ctx->source->source.directory_path);
        source_tables = load_from_directory(ctx->source->source.directory_path, &source_count, NULL);
        if (source_tables) {
            log_info("Loaded %d tables from source directory", source_count);
        } else {
            log_error("Failed to load tables from source directory");
        }
    } else if (ctx->source->type == SOURCE_TYPE_FILE) {
        log_info("Loading source from file: %s", ctx->source->source.file_path);
        source_tables = load_from_file(ctx->source->source.file_path, &source_count, NULL);
        if (source_tables) {
            log_info("Loaded %d tables from source file", source_count);
        }
    }

    /* Load target */
    if (ctx->target->type == SOURCE_TYPE_DATABASE) {
        log_info("Connecting to target database: %s@%s:%s/%s",
                ctx->target->source.db_config.user ? ctx->target->source.db_config.user : "default",
                ctx->target->source.db_config.host,
                ctx->target->source.db_config.port,
                ctx->target->source.db_config.database);

        target_conn = db_connect(&ctx->target->source.db_config);
        if (!target_conn || !db_is_connected(target_conn)) {
            log_error("Failed to connect to target database: %s",
                     target_conn ? db_get_error(target_conn) : "connection failed");
        } else {
            const char *schema = ctx->target->schema_name ? ctx->target->schema_name : "public";
            target_tables = load_from_database(target_conn, schema, &target_count, NULL);
            if (target_tables) {
                log_info("Loaded %d tables from target database", target_count);
            } else {
                log_error("Failed to load tables from target database");
            }
        }
    } else if (ctx->target->type == SOURCE_TYPE_DIRECTORY) {
        log_info("Loading target from directory: %s", ctx->target->source.directory_path);
        target_tables = load_from_directory(ctx->target->source.directory_path, &target_count, NULL);
        if (target_tables) {
            log_info("Loaded %d tables from target directory", target_count);
        } else {
            log_error("Failed to load tables from target directory");
        }
    } else if (ctx->target->type == SOURCE_TYPE_FILE) {
        log_info("Loading target from file: %s", ctx->target->source.file_path);
        target_tables = load_from_file(ctx->target->source.file_path, &target_count, NULL);
        if (target_tables) {
            log_info("Loaded %d tables from target file", target_count);
        }
    }

    /* Print summary */
    printf("\nSummary:\n");
    printf("  Source: %d tables loaded\n", source_count);
    printf("  Target: %d tables loaded\n", target_count);

    int result = 0;

    /* Perform comparison if at least one schema loaded */
    if ((source_tables && source_count > 0) || (target_tables && target_count > 0)) {
        log_info("Comparing schemas...");

        SchemaDiff *diff = compare_schemas(source_tables, source_count,
                                          target_tables, target_count,
                                          ctx->compare_opts, NULL);

        if (!diff) {
            log_error("Failed to compare schemas");
            result = 1;
        } else {
            log_info("Comparison complete");

            /* Generate report if requested */
            if (ctx->generate_report) {
                char *report = generate_report(diff, ctx->report_opts);
                if (!report) {
                    log_error("Failed to generate report");
                    result = 1;
                } else {
                    if (ctx->report_output_file) {
                        if (write_string_to_file(ctx->report_output_file, report)) {
                            printf("Report written to: %s\n", ctx->report_output_file);
                        } else {
                            log_error("Failed to write report to file: %s", ctx->report_output_file);
                            result = 1;
                        }
                    } else {
                        /* Print to stdout */
                        printf("\n%s", report);
                    }
                    free(report);
                }
            }

            /* Generate SQL migration if requested */
            if (ctx->generate_sql) {
                log_info("Generating SQL migration script...");

                SQLMigration *migration = generate_migration_sql(diff, ctx->sql_opts);
                if (!migration) {
                    log_error("Failed to generate SQL migration");
                    result = 1;
                } else {
                    /* Check if we need to prepend CREATE DATABASE */
                    char *final_sql = NULL;
                    bool need_create_db = false;

                    /* If target has a database name and source is a database that doesn't have it */
                    if (ctx->target->database_name && ctx->source->type == SOURCE_TYPE_DATABASE) {
                        /* Check if the target database name differs from source */
                        if (!ctx->source->database_name ||
                            strcmp(ctx->target->database_name, ctx->source->database_name) != 0) {
                            need_create_db = true;
                        }
                    }

                    if (need_create_db) {
                        /* Prepend CREATE DATABASE statement */
                        StringBuilder *sb = sb_create();
                        sb_append(sb, "-- Create database if it doesn't exist\n");
                        sb_append(sb, "-- Note: This must be run in a separate transaction from the rest of the migration\n");
                        sb_append_fmt(sb, "CREATE DATABASE \"%s\";\n\n", ctx->target->database_name);
                        sb_append_fmt(sb, "-- Connect to the database:\n");
                        sb_append_fmt(sb, "-- \\c %s\n\n", ctx->target->database_name);
                        sb_append(sb, migration->forward_sql);
                        final_sql = sb_to_string(sb);
                        sb_free(sb);

                        printf("⚠ Note: Migration includes CREATE DATABASE for '%s'\n",
                               ctx->target->database_name);
                        printf("   You must run the CREATE DATABASE statement separately first.\n");
                    } else {
                        final_sql = strdup(migration->forward_sql);
                    }

                    if (ctx->sql_output_file) {
                        /* Write to file */
                        if (write_string_to_file(ctx->sql_output_file, final_sql)) {
                            printf("SQL migration written to: %s\n", ctx->sql_output_file);
                            log_info("SQL migration generated successfully");
                            if (migration->has_destructive_changes) {
                                printf("⚠ Warning: Migration contains destructive changes\n");
                            }
                            printf("Generated %d SQL statements\n", migration->statement_count);
                        } else {
                            log_error("Failed to write SQL to file: %s", ctx->sql_output_file);
                            result = 1;
                        }
                    } else {
                        /* Print to stdout */
                        printf("\n-- SQL Migration Script\n");
                        printf("-- Generated by schema-compare\n");
                        if (migration->has_destructive_changes) {
                            printf("-- ⚠ Warning: Contains destructive changes\n");
                        }
                        printf("-- Statements: %d\n\n", migration->statement_count);
                        printf("%s\n", final_sql);
                    }

                    free(final_sql);
                    sql_migration_free(migration);
                }
            }

            schema_diff_free(diff);
        }
    } else {
        log_error("No tables loaded from either source or target");
        result = 1;
    }

    /* Cleanup */
    if (source_conn) {
        db_disconnect(source_conn);
    }

    if (target_conn) {
        db_disconnect(target_conn);
    }

    if (source_tables) {
        for (int i = 0; i < source_count; i++) {
            free_create_table_stmt(source_tables[i]);
        }
        free(source_tables);
    }

    if (target_tables) {
        for (int i = 0; i < target_count; i++) {
            free_create_table_stmt(target_tables[i]);
        }
        free(target_tables);
    }

    app_context_free(ctx);
    log_shutdown();

    return result;
}
