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
    printf("Usage: %s --source SOURCE --target TARGET [--target TARGET2 ...] [OPTIONS]\n\n", program_name);
    printf("Compare PostgreSQL schemas and generate migration scripts.\n\n");
    printf("Required Arguments:\n");
    printf("  --source SOURCE      Source schema (PostgreSQL URI or directory path)\n");
    printf("  --target TARGET      Target database (PostgreSQL URI, can specify multiple times)\n\n");
    printf("Options:\n");
    printf("  -o, --output FILE        Write migration to FILE (single target only)\n");
    printf("  -s, --sql [FILE]         Generate SQL migration script (to FILE or stdout)\n");
    printf("  -f, --format FORMAT      Report format: text, markdown (default: text)\n");
    printf("  -v, --verbose            Verbose output\n");
    printf("  -q, --quiet              Quiet mode (errors only)\n");
    printf("  --no-color               Disable colored output\n");
    printf("  --no-transactions        Don't wrap SQL in transactions\n");
    printf("  --schema NAME            Schema name for database sources (default: public)\n");
    printf("  -h, --help               Show this help message\n");
    printf("  -V, --version            Show version information\n\n");
    printf("PostgreSQL Connection URIs:\n");
    printf("  Use standard PostgreSQL connection URI format:\n");
    printf("    postgresql://user:password@host:port/database\n");
    printf("    postgresql://user@host/database  (password prompted)\n");
    printf("    postgresql://host/database       (default user)\n\n");
    printf("Examples:\n");
    printf("  Compare directory to single database:\n");
    printf("    %s --source ./schema/ --target postgresql://user:pass@localhost:5432/mydb\n\n", program_name);
    printf("  Compare directory to multiple databases:\n");
    printf("    %s --source ./schema/ --target postgresql://user:pass@host1:5432/db1 \\\n", program_name);
    printf("                                     --target postgresql://user:pass@host2:5432/db2\n");
    printf("    (Generates: migration-db1.sql and migration-db2.sql)\n\n");
    printf("  Compare database to database with custom output:\n");
    printf("    %s --source postgresql://user:pass@prod:5432/app \\\n", program_name);
    printf("                --target postgresql://user:pass@staging:5432/app --output sync.sql\n\n");
    printf("  Generate migration SQL to stdout:\n");
    printf("    %s --sql --source ./schema/ --target postgresql://localhost/mydb\n\n", program_name);
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

/* Check if string looks like a PostgreSQL URI */
static bool is_postgresql_uri(const char *str) {
    return (strncmp(str, "postgresql://", 13) == 0 || strncmp(str, "postgres://", 11) == 0);
}

/* Parse PostgreSQL URI into DBConfig
 * Format: postgresql://[user[:password]@][host[:port]][/database]
 */
static DBConfig parse_postgresql_uri(const char *uri) {
    DBConfig config = {0};

    /* Set defaults */
    config.host = strdup("localhost");
    config.port = strdup("5432");
    config.database = NULL;
    config.user = NULL;
    config.password = NULL;
    config.connect_timeout = 30;

    /* Skip protocol */
    const char *p = uri;
    if (strncmp(p, "postgresql://", 13) == 0) {
        p += 13;
    } else if (strncmp(p, "postgres://", 11) == 0) {
        p += 11;
    }

    /* Parse user:password@ if present */
    const char *at = strchr(p, '@');
    if (at) {
        /* Extract user:password part */
        size_t userpass_len = at - p;
        char *userpass = strndup(p, userpass_len);

        char *colon = strchr(userpass, ':');
        if (colon) {
            *colon = '\0';
            config.user = strdup(userpass);
            config.password = strdup(colon + 1);
        } else {
            config.user = strdup(userpass);
        }

        free(userpass);
        p = at + 1;
    }

    /* Parse host:port/database */
    const char *slash = strchr(p, '/');
    const char *colon = strchr(p, ':');

    if (slash) {
        /* Database specified */
        config.database = strdup(slash + 1);

        /* Parse host:port before the slash */
        size_t hostport_len = slash - p;
        char *hostport = strndup(p, hostport_len);

        if (colon && colon < slash) {
            /* Port specified */
            size_t host_len = colon - p;
            free(config.host);
            config.host = strndup(p, host_len);

            const char *port_start = colon + 1;
            size_t port_len = slash - port_start;
            free(config.port);
            config.port = strndup(port_start, port_len);
        } else if (hostport_len > 0) {
            /* Only host specified */
            free(config.host);
            config.host = hostport;
            hostport = NULL;
        }

        free(hostport);
    } else if (colon) {
        /* host:port without database */
        size_t host_len = colon - p;
        free(config.host);
        config.host = strndup(p, host_len);
        free(config.port);
        config.port = strdup(colon + 1);
    } else if (*p) {
        /* Only host specified */
        free(config.host);
        config.host = strdup(p);
    }

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

    /* Look for "src" directory, then extract database/schema after it */
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
            *schema = strdup("public");
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
    if (is_postgresql_uri(spec)) {
        source->type = SOURCE_TYPE_DATABASE;
        source->source.db_config = parse_postgresql_uri(spec);

        /* Validate required fields */
        if (!source->source.db_config.database) {
            fprintf(stderr, "Error: Database name is required in PostgreSQL URI\n");
            fprintf(stderr, "Format: postgresql://user:pass@host:port/database\n");
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
        fprintf(stderr, "Error: Invalid source specification: %s\n", spec);
        fprintf(stderr, "Must be a PostgreSQL URI, directory path, or SQL file\n");
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

    ctx->generate_sql = true;
    ctx->generate_report = true;
    ctx->verbose = false;
    ctx->quiet = false;

    ctx->targets = NULL;
    ctx->target_count = 0;

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

    /* Free all targets */
    for (int i = 0; i < ctx->target_count; i++) {
        schema_source_free(ctx->targets[i]);
    }
    free(ctx->targets);

    free(ctx);
}

/* Validate source/target constraints */
bool validate_source_target_constraints(AppContext *ctx) {
    /* Source can be directory, file, or database (already validated) */

    /* All targets MUST be databases */
    for (int i = 0; i < ctx->target_count; i++) {
        if (ctx->targets[i]->type != SOURCE_TYPE_DATABASE) {
            fprintf(stderr, "Error: Target #%d is not a database connection.\n", i + 1);
            fprintf(stderr, "Targets must be PostgreSQL database URIs.\n");
            fprintf(stderr, "Format: postgresql://user:pass@host:port/database\n");
            return false;
        }
    }

    return true;
}

/* Parse command line arguments */
AppContext *parse_command_line(int argc, char **argv) {
    AppContext *ctx = app_context_create();
    if (!ctx) {
        return NULL;
    }

    static struct option long_options[] = {
        {"source",          required_argument, 0, 1000},  // Long-only option
        {"target",          required_argument, 0, 't'},
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

    /* Track source and targets */
    char *source_arg = NULL;
    char **target_args = NULL;
    int target_capacity = 4;
    int target_count = 0;

    target_args = malloc(sizeof(char*) * target_capacity);
    if (!target_args) {
        app_context_free(ctx);
        return NULL;
    }

    while ((opt = getopt_long(argc, argv, "t:o:s::f:vqhV", long_options, &option_index)) != -1) {
        switch (opt) {
            case 1000:  // --source
                if (source_arg) {
                    fprintf(stderr, "Error: --source can only be specified once\n");
                    free(target_args);
                    app_context_free(ctx);
                    return NULL;
                }
                source_arg = optarg;
                break;
            case 't':  // --target
                /* Expand array if needed */
                if (target_count >= target_capacity) {
                    target_capacity *= 2;
                    char **new_targets = realloc(target_args, sizeof(char*) * target_capacity);
                    if (!new_targets) {
                        free(target_args);
                        app_context_free(ctx);
                        return NULL;
                    }
                    target_args = new_targets;
                }
                target_args[target_count++] = optarg;
                break;
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
            case 'S':
                /* Schema option - will be used when loading databases */
                ctx->schema_name_override = optarg;
                break;
            case 'h':
                print_usage(argv[0]);
                free(target_args);
                app_context_free(ctx);
                exit(0);
            case 'V':
                print_version();
                free(target_args);
                app_context_free(ctx);
                exit(0);
            default:
                print_usage(argv[0]);
                free(target_args);
                app_context_free(ctx);
                return NULL;
        }
    }

    /* Validate required arguments */
    if (!source_arg) {
        fprintf(stderr, "Error: --source is required\n\n");
        print_usage(argv[0]);
        free(target_args);
        app_context_free(ctx);
        return NULL;
    }

    if (target_count == 0) {
        fprintf(stderr, "Error: At least one --target is required\n\n");
        print_usage(argv[0]);
        free(target_args);
        app_context_free(ctx);
        return NULL;
    }

    /* Parse source */
    ctx->source = parse_schema_source(source_arg);
    if (!ctx->source) {
        fprintf(stderr, "Error: Failed to parse source specification\n");
        free(target_args);
        app_context_free(ctx);
        return NULL;
    }

    /* Parse all targets */
    ctx->targets = malloc(sizeof(SchemaSource*) * target_count);
    if (!ctx->targets) {
        free(target_args);
        app_context_free(ctx);
        return NULL;
    }

    ctx->target_count = target_count;
    for (int i = 0; i < target_count; i++) {
        ctx->targets[i] = parse_schema_source(target_args[i]);
        if (!ctx->targets[i]) {
            fprintf(stderr, "Error: Failed to parse target #%d specification\n", i + 1);
            free(target_args);
            app_context_free(ctx);
            return NULL;
        }
    }

    free(target_args);

    /* Validate source/target constraints */
    if (!validate_source_target_constraints(ctx)) {
        app_context_free(ctx);
        return NULL;
    }

    /* Warn if --output specified with multiple targets */
    if (ctx->target_count > 1 && ctx->report_output_file) {
        fprintf(stderr, "Warning: --output is ignored when multiple targets are specified.\n");
        fprintf(stderr, "         Migration files will be named migration-[database].sql\n\n");
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

/* Load schemas from file */
CreateTableStmt **load_from_file(const char *file_path, int *table_count,
                                 MemoryContext *mem_ctx) {
    (void)mem_ctx;

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

/* Load schemas from directory */
CreateTableStmt **load_from_directory(const char *dir_path, int *table_count,
                                      MemoryContext *mem_ctx) {
    (void)mem_ctx;

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

/* Generate migration filename for a target database */
char* generate_migration_filename(const char *database_name,
                                   const char *output_arg,
                                   int target_count) {
    /* Single target with --output specified: use specified filename */
    if (target_count == 1 && output_arg) {
        return strdup(output_arg);
    }

    /* Otherwise: migration-[database-name].sql */
    /* Sanitize database name for filename */
    char safe_name[256];
    strncpy(safe_name, database_name, sizeof(safe_name) - 1);
    safe_name[sizeof(safe_name) - 1] = '\0';

    /* Replace invalid filename chars with underscores */
    for (char *p = safe_name; *p; p++) {
        if (*p == '/' || *p == '\\' || *p == ':' || *p == '*' ||
            *p == '?' || *p == '"' || *p == '<' || *p == '>' || *p == '|') {
            *p = '_';
        }
    }

    char *filename = malloc(strlen(safe_name) + 20);
    if (!filename) {
        return NULL;
    }
    sprintf(filename, "migration-%s.sql", safe_name);
    return filename;
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

    /* Load source schema */
    int source_count = 0;
    CreateTableStmt **source_tables = NULL;
    DBConnection *source_conn = NULL;

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
            app_context_free(ctx);
            log_shutdown();
            return 1;
        }

        const char *schema = ctx->schema_name_override ? ctx->schema_name_override :
                            (ctx->source->schema_name ? ctx->source->schema_name : "public");
        source_tables = load_from_database(source_conn, schema, &source_count, NULL);
        if (source_tables) {
            log_info("Loaded %d tables from source database", source_count);
        } else {
            log_error("Failed to load tables from source database");
            db_disconnect(source_conn);
            app_context_free(ctx);
            log_shutdown();
            return 1;
        }
    } else if (ctx->source->type == SOURCE_TYPE_DIRECTORY) {
        log_info("Loading source from directory: %s", ctx->source->source.directory_path);
        source_tables = load_from_directory(ctx->source->source.directory_path, &source_count, NULL);
        if (source_tables) {
            log_info("Loaded %d tables from source directory", source_count);
        } else {
            log_error("Failed to load tables from source directory");
            app_context_free(ctx);
            log_shutdown();
            return 1;
        }
    } else if (ctx->source->type == SOURCE_TYPE_FILE) {
        log_info("Loading source from file: %s", ctx->source->source.file_path);
        source_tables = load_from_file(ctx->source->source.file_path, &source_count, NULL);
        if (!source_tables) {
            log_error("Failed to load tables from source file");
            app_context_free(ctx);
            log_shutdown();
            return 1;
        }
        log_info("Loaded %d tables from source file", source_count);
    }

    int result = 0;
    int successful_migrations = 0;

    /* Process each target */
    for (int target_idx = 0; target_idx < ctx->target_count; target_idx++) {
        SchemaSource *target = ctx->targets[target_idx];

        printf("\n=== Processing target %d/%d: %s ===\n",
               target_idx + 1, ctx->target_count, target->database_name);

        /* Load target database */
        int target_count = 0;
        CreateTableStmt **target_tables = NULL;
        DBConnection *target_conn = NULL;

        log_info("Connecting to target database: %s@%s:%s/%s",
                target->source.db_config.user ? target->source.db_config.user : "default",
                target->source.db_config.host,
                target->source.db_config.port,
                target->source.db_config.database);

        target_conn = db_connect(&target->source.db_config);
        if (!target_conn || !db_is_connected(target_conn)) {
            log_error("Failed to connect to target database #%d: %s",
                     target_idx + 1,
                     target_conn ? db_get_error(target_conn) : "connection failed");
            if (target_conn) {
                db_disconnect(target_conn);
            }
            result = 1;
            continue;
        }

        const char *schema = ctx->schema_name_override ? ctx->schema_name_override :
                            (target->schema_name ? target->schema_name : "public");
        target_tables = load_from_database(target_conn, schema, &target_count, NULL);
        if (!target_tables) {
            log_error("Failed to load tables from target database #%d", target_idx + 1);
            db_disconnect(target_conn);
            result = 1;
            continue;
        }

        log_info("Loaded %d tables from target database", target_count);

        /* Compare schemas */
        log_info("Comparing schemas...");
        SchemaDiff *diff = compare_schemas(source_tables, source_count,
                                          target_tables, target_count,
                                          ctx->compare_opts, NULL);

        if (!diff) {
            log_error("Failed to compare schemas for target #%d", target_idx + 1);
            for (int i = 0; i < target_count; i++) {
                free_create_table_stmt(target_tables[i]);
            }
            free(target_tables);
            db_disconnect(target_conn);
            result = 1;
            continue;
        }

        log_info("Comparison complete");

        /* Generate migration SQL */
        if (ctx->generate_sql || ctx->sql_output_file) {
            log_info("Generating SQL migration script...");

            SQLMigration *migration = generate_migration_sql(diff, ctx->sql_opts);
            if (!migration) {
                log_error("Failed to generate SQL migration for target #%d", target_idx + 1);
                schema_diff_free(diff);
                for (int i = 0; i < target_count; i++) {
                    free_create_table_stmt(target_tables[i]);
                }
                free(target_tables);
                db_disconnect(target_conn);
                result = 1;
                continue;
            }

            /* Generate output filename */
            char *output_filename = generate_migration_filename(
                target->database_name,
                ctx->report_output_file,
                ctx->target_count
            );

            if (!output_filename) {
                log_error("Failed to generate output filename for target #%d", target_idx + 1);
                sql_migration_free(migration);
                schema_diff_free(diff);
                for (int i = 0; i < target_count; i++) {
                    free_create_table_stmt(target_tables[i]);
                }
                free(target_tables);
                db_disconnect(target_conn);
                result = 1;
                continue;
            }

            /* Write migration to file */
            if (write_string_to_file(output_filename, migration->forward_sql)) {
                printf("✓ Migration written to: %s\n", output_filename);
                log_info("SQL migration generated successfully");
                if (migration->has_destructive_changes) {
                    printf("  ⚠ Warning: Migration contains destructive changes\n");
                }
                printf("  Generated %d SQL statements\n", migration->statement_count);
                successful_migrations++;
            } else {
                log_error("Failed to write SQL to file: %s", output_filename);
                result = 1;
            }

            free(output_filename);
            sql_migration_free(migration);
        }

        /* Generate report if requested */
        if (ctx->generate_report && ctx->target_count == 1) {
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
                    printf("\n%s", report);
                }
                free(report);
            }
        }

        schema_diff_free(diff);

        /* Cleanup target resources */
        for (int i = 0; i < target_count; i++) {
            free_create_table_stmt(target_tables[i]);
        }
        free(target_tables);
        db_disconnect(target_conn);
    }

    /* Print summary */
    printf("\n=== Summary ===\n");
    printf("Source: %d tables loaded\n", source_count);
    printf("Targets processed: %d/%d\n", successful_migrations, ctx->target_count);
    if (successful_migrations < ctx->target_count) {
        printf("⚠ Some targets failed - check logs above\n");
    }

    /* Cleanup source resources */
    if (source_conn) {
        db_disconnect(source_conn);
    }

    if (source_tables) {
        for (int i = 0; i < source_count; i++) {
            free_create_table_stmt(source_tables[i]);
        }
        free(source_tables);
    }

    app_context_free(ctx);
    log_shutdown();

    return result;
}
