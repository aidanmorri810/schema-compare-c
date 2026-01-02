#include "db_reader.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Build connection info string from config */
char *db_build_conninfo(const DBConfig *config) {
    if (!config) {
        return NULL;
    }

    StringBuilder *sb = sb_create();
    if (!sb) {
        return NULL;
    }

    if (config->host) {
        sb_append_fmt(sb, "host=%s ", config->host);
    }
    if (config->port) {
        sb_append_fmt(sb, "port=%s ", config->port);
    }
    if (config->database) {
        sb_append_fmt(sb, "dbname=%s ", config->database);
    }
    if (config->user) {
        sb_append_fmt(sb, "user=%s ", config->user);
    }
    if (config->password) {
        sb_append_fmt(sb, "password=%s ", config->password);
    }
    if (config->connect_timeout > 0) {
        sb_append_fmt(sb, "connect_timeout=%d ", config->connect_timeout);
    }

    char *conninfo = sb_to_string(sb);
    sb_free(sb);

    return conninfo;
}

/* Connect to database */
DBConnection *db_connect(const DBConfig *config) {
    if (!config) {
        return NULL;
    }

    DBConnection *conn = calloc(1, sizeof(DBConnection));
    if (!conn) {
        return NULL;
    }

    /* Copy config */
    conn->config.host = config->host ? strdup(config->host) : NULL;
    conn->config.port = config->port ? strdup(config->port) : NULL;
    conn->config.database = config->database ? strdup(config->database) : NULL;
    conn->config.user = config->user ? strdup(config->user) : NULL;
    conn->config.password = config->password ? strdup(config->password) : NULL;
    conn->config.connect_timeout = config->connect_timeout;

    /* Build connection string */
    char *conninfo = db_build_conninfo(config);
    if (!conninfo) {
        free(conn);
        return NULL;
    }

    /* Connect */
    conn->conn = PQconnectdb(conninfo);
    free(conninfo);

    if (PQstatus(conn->conn) != CONNECTION_OK) {
        conn->last_error = strdup(PQerrorMessage(conn->conn));
        conn->connected = false;
        log_error("Database connection failed: %s", conn->last_error);
        return conn;
    }

    conn->connected = true;
    log_info("Connected to database: %s", config->database ? config->database : "default");

    return conn;
}

/* Disconnect from database */
void db_disconnect(DBConnection *conn) {
    if (!conn) {
        return;
    }

    if (conn->conn) {
        PQfinish(conn->conn);
        conn->conn = NULL;
    }

    free(conn->config.host);
    free(conn->config.port);
    free(conn->config.database);
    free(conn->config.user);
    free(conn->config.password);
    free(conn->last_error);
    free(conn);
}

/* Check if connected */
bool db_is_connected(DBConnection *conn) {
    if (!conn || !conn->conn) {
        return false;
    }

    return PQstatus(conn->conn) == CONNECTION_OK;
}

/* Get last error message */
const char *db_get_error(DBConnection *conn) {
    if (!conn) {
        return "Connection is NULL";
    }

    if (conn->last_error) {
        return conn->last_error;
    }

    if (conn->conn) {
        return PQerrorMessage(conn->conn);
    }

    return "Unknown error";
}

/* Escape identifier for SQL */
char *db_escape_identifier(const char *identifier) {
    if (!identifier) {
        return NULL;
    }

    /* Simple escaping - wrap in double quotes and escape internal quotes */
    size_t len = strlen(identifier);
    size_t escaped_len = len * 2 + 3; /* Worst case: all quotes + surrounding quotes + null */
    char *escaped = malloc(escaped_len);
    if (!escaped) {
        return NULL;
    }

    char *dst = escaped;
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

    return escaped;
}
