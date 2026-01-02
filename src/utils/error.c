#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Create error object */
Error *error_create(ErrorCode code, const char *message, const char *details) {
    Error *err = malloc(sizeof(Error));
    if (!err) {
        return NULL;
    }

    err->code = code;
    err->message = message ? strdup(message) : NULL;
    err->details = details ? strdup(details) : NULL;
    err->file = NULL;
    err->line = 0;

    return err;
}

/* Free error object */
void error_free(Error *err) {
    if (!err) {
        return;
    }

    free(err->message);
    free(err->details);
    free(err);
}

/* Print error to stderr */
void error_print(const Error *err) {
    if (!err) {
        return;
    }

    fprintf(stderr, "Error [%s]: %s\n",
            error_code_to_string(err->code),
            err->message ? err->message : "Unknown error");

    if (err->details) {
        fprintf(stderr, "  Details: %s\n", err->details);
    }

    if (err->file) {
        fprintf(stderr, "  Location: %s:%d\n", err->file, err->line);
    }
}

/* Convert error code to string */
const char *error_code_to_string(ErrorCode code) {
    switch (code) {
        case ERROR_NONE:
            return "NONE";
        case ERROR_PARSE:
            return "PARSE";
        case ERROR_DB_CONNECTION:
            return "DB_CONNECTION";
        case ERROR_DB_QUERY:
            return "DB_QUERY";
        case ERROR_FILE_IO:
            return "FILE_IO";
        case ERROR_MEMORY:
            return "MEMORY";
        case ERROR_INVALID_ARG:
            return "INVALID_ARG";
        default:
            return "UNKNOWN";
    }
}

/* Logging implementation */
static struct {
    FILE *log_file;
    LogLevel min_level;
    bool initialized;
} log_state = {NULL, LOG_LEVEL_INFO, false};

void log_init(const char *filename, LogLevel min_level) {
    if (log_state.initialized) {
        log_shutdown();
    }

    if (filename) {
        log_state.log_file = fopen(filename, "a");
        if (!log_state.log_file) {
            log_state.log_file = stderr;
        }
    } else {
        log_state.log_file = stderr;
    }

    log_state.min_level = min_level;
    log_state.initialized = true;
}

void log_message(LogLevel level, const char *format, ...) {
    if (!log_state.initialized) {
        log_init(NULL, LOG_LEVEL_INFO);
    }

    if (level < log_state.min_level) {
        return;
    }

    const char *level_str;
    switch (level) {
        case LOG_LEVEL_DEBUG:
            level_str = "DEBUG";
            break;
        case LOG_LEVEL_INFO:
            level_str = "INFO";
            break;
        case LOG_LEVEL_WARN:
            level_str = "WARN";
            break;
        case LOG_LEVEL_ERROR:
            level_str = "ERROR";
            break;
        default:
            level_str = "UNKNOWN";
    }

    fprintf(log_state.log_file, "[%s] ", level_str);

    va_list args;
    va_start(args, format);
    vfprintf(log_state.log_file, format, args);
    va_end(args);

    fprintf(log_state.log_file, "\n");
    fflush(log_state.log_file);
}

void log_debug(const char *format, ...) {
    if (!log_state.initialized || LOG_LEVEL_DEBUG < log_state.min_level) {
        return;
    }

    fprintf(log_state.log_file, "[DEBUG] ");

    va_list args;
    va_start(args, format);
    vfprintf(log_state.log_file, format, args);
    va_end(args);

    fprintf(log_state.log_file, "\n");
    fflush(log_state.log_file);
}

void log_info(const char *format, ...) {
    if (!log_state.initialized || LOG_LEVEL_INFO < log_state.min_level) {
        return;
    }

    fprintf(log_state.log_file, "[INFO] ");

    va_list args;
    va_start(args, format);
    vfprintf(log_state.log_file, format, args);
    va_end(args);

    fprintf(log_state.log_file, "\n");
    fflush(log_state.log_file);
}

void log_warn(const char *format, ...) {
    if (!log_state.initialized || LOG_LEVEL_WARN < log_state.min_level) {
        return;
    }

    fprintf(log_state.log_file, "[WARN] ");

    va_list args;
    va_start(args, format);
    vfprintf(log_state.log_file, format, args);
    va_end(args);

    fprintf(log_state.log_file, "\n");
    fflush(log_state.log_file);
}

void log_error(const char *format, ...) {
    if (!log_state.initialized || LOG_LEVEL_ERROR < log_state.min_level) {
        return;
    }

    fprintf(log_state.log_file, "[ERROR] ");

    va_list args;
    va_start(args, format);
    vfprintf(log_state.log_file, format, args);
    va_end(args);

    fprintf(log_state.log_file, "\n");
    fflush(log_state.log_file);
}

void log_shutdown(void) {
    if (!log_state.initialized) {
        return;
    }

    if (log_state.log_file && log_state.log_file != stderr) {
        fclose(log_state.log_file);
    }

    log_state.log_file = NULL;
    log_state.initialized = false;
}
