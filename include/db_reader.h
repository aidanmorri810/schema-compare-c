#ifndef DB_READER_H
#define DB_READER_H

#include "pg_schema.h"
#include "pg_create_table.h"
#include "sc_memory.h"
#include <libpq-fe.h>
#include <stdbool.h>
#include "pg_schema.h"

/* Database connection configuration */
typedef struct {
    char *host;
    char *port;
    char *database;
    char *user;
    char *password;
    int connect_timeout;
} DBConfig;

/* Database connection handle */
typedef struct {
    PGconn *conn;
    DBConfig config;
    bool connected;
    char *last_error;
} DBConnection;

/* Schema introspection options */
typedef struct {
    bool include_system_tables;
    bool include_temp_tables;
    bool include_unlogged_tables;
    char **schemas;
    int schema_count;
} IntrospectionOptions;

Schema *db_read_schema(DBConnection *conn, const char *schema_name,
                               MemoryContext *mem_ctx);


/*                    */
/* Main DB Reader API */
/*                    */
DBConnection *db_connect(const DBConfig *config);
void db_disconnect(DBConnection *conn);
bool db_is_connected(DBConnection *conn);
const char *db_get_error(DBConnection *conn);
char *db_escape_identifier(const char *identifier);
char *db_build_conninfo(const DBConfig *config);

/* db_table.c */
CreateTableStmt **db_read_all_tables(DBConnection *conn, const char *schema_name,
                                        int *table_count, MemoryContext *mem_ctx);
CreateTableStmt *db_read_table(DBConnection *conn, const char *schema,
                               const char *table_name, MemoryContext *mem_ctx);
bool db_populate_table_info(DBConnection *conn, const char *schema,
                            CreateTableStmt **stmts, int stmt_count,
                            MemoryContext *mem_ctx);

/* db_columns.c */
bool db_populate_columns(DBConnection *conn, const char *schema,
                        CreateTableStmt **stmts, int stmt_count,
                        MemoryContext *mem_ctx);

/* db_constraint.c */
bool db_populate_constraints(DBConnection *conn, const char *schema,
                             CreateTableStmt **stmts, int stmt_count,
                             MemoryContext *mem_ctx);


#endif /* DB_READER_H */
