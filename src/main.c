#include "schema_compare.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

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
    printf("  -s, --sql FILE           Generate SQL migration script to FILE\n");
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
    printf("  Generate migration SQL:\n");
    printf("    %s --sql migration.sql schema_v1.sql schema_v2.sql\n\n", program_name);
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

    /* Note: source and target are not owned by ctx in this simple version */
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
        {"sql",             required_argument, 0, 's'},
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

    while ((opt = getopt_long(argc, argv, "o:s:f:vqhV", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'o':
                ctx->report_output_file = optarg;
                break;
            case 's':
                ctx->sql_output_file = optarg;
                ctx->generate_sql = true;
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

    /* For now, just store the argument strings */
    /* TODO: Parse source/target specifications */
    (void)argv;  /* Suppress unused warning for now */

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
    free(source);

    if (!parser) {
        *table_count = 0;
        return NULL;
    }

    CreateTableStmt *stmt = parser_parse_create_table(parser);
    parser_destroy(parser);

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

/* Load schemas from directory */
CreateTableStmt **load_from_directory(const char *dir_path, int *table_count,
                                      MemoryContext *mem_ctx) {
    (void)dir_path;
    (void)table_count;
    (void)mem_ctx;
    /* TODO: Implement directory scanning */
    return NULL;
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

    /* TODO: Implement main workflow */
    printf("schema-compare v%s\n", SCHEMA_COMPARE_VERSION);
    printf("Note: Full CLI implementation in progress\n");
    printf("\n");
    printf("Current status:\n");
    printf("  ✓ Parser module - Complete\n");
    printf("  ✓ Database reader - Complete\n");
    printf("  ✓ Comparison engine - Complete\n");
    printf("  ✓ Report generator - Complete\n");
    printf("  ✓ SQL generator - Complete\n");
    printf("  → CLI integration - In progress\n");

    /* Cleanup */
    app_context_free(ctx);
    log_shutdown();

    return 0;
}
