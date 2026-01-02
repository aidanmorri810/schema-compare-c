#ifndef SCHEMA_COMPARE_H
#define SCHEMA_COMPARE_H

#include "pg_create_table.h"
#include "parser.h"
#include "db_reader.h"
#include "compare.h"
#include "report.h"
#include "sql_generator.h"
#include "sc_memory.h"
#include <stdbool.h>

/* Application version */
#define SCHEMA_COMPARE_VERSION "0.1.0"

/* Source type for comparison */
typedef enum {
    SOURCE_TYPE_DATABASE,    /* PostgreSQL database */
    SOURCE_TYPE_FILE,        /* Single DDL file */
    SOURCE_TYPE_DIRECTORY    /* Directory of DDL files */
} SourceType;

/* Schema source specification */
typedef struct {
    SourceType type;
    union {
        DBConfig db_config;      /* For SOURCE_TYPE_DATABASE */
        char *file_path;         /* For SOURCE_TYPE_FILE */
        char *directory_path;    /* For SOURCE_TYPE_DIRECTORY */
    } source;
    char *database_name;         /* Database name (extracted from directory or connection) */
    char *schema_name;           /* Schema name (for database sources) */
} SchemaSource;

/* Application context */
typedef struct {
    SchemaSource *source;
    SchemaSource *target;
    CompareOptions *compare_opts;
    ReportOptions *report_opts;
    SQLGenOptions *sql_opts;

    bool generate_sql;
    char *sql_output_file;

    bool generate_report;
    char *report_output_file;

    bool verbose;
    bool quiet;
} AppContext;

/* Initialize and free application context */
AppContext *app_context_create(void);
void app_context_free(AppContext *ctx);

/* Parse command line arguments */
AppContext *parse_command_line(int argc, char **argv);

/* Parse schema source specification */
SchemaSource *parse_schema_source(const char *spec);
void schema_source_free(SchemaSource *source);

/* Print usage information */
void print_usage(const char *program_name);
void print_version(void);

/* Main comparison workflows */
int compare_and_report(SchemaSource *source, SchemaSource *target, AppContext *ctx);

/* Load schemas from sources */
CreateTableStmt **load_from_database(DBConnection *conn, const char *schema,
                                     int *table_count, MemoryContext *mem_ctx);
CreateTableStmt **load_from_file(const char *file_path, int *table_count,
                                 MemoryContext *mem_ctx);
CreateTableStmt **load_from_directory(const char *dir_path, int *table_count,
                                      MemoryContext *mem_ctx);

#endif /* SCHEMA_COMPARE_H */
