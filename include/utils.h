#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>

/* String utilities */
char *str_trim(const char *str);
char *str_remove_whitespace(const char *str);
char *str_to_upper(const char *str);
char *str_to_lower(const char *str);
bool str_equals_ignore_case(const char *s1, const char *s2);
char *str_concat(const char *s1, const char *s2);
char **str_split(const char *str, char delimiter, int *count);
void str_free_array(char **arr, int count);

/* String builder for efficient concatenation */
typedef struct StringBuilder StringBuilder;

StringBuilder *sb_create(void);
void sb_append(StringBuilder *sb, const char *str);
void sb_append_char(StringBuilder *sb, char c);
void sb_append_fmt(StringBuilder *sb, const char *format, ...);
char *sb_to_string(StringBuilder *sb);
void sb_free(StringBuilder *sb);

/* Error handling */
typedef enum {
    ERROR_NONE,
    ERROR_PARSE,
    ERROR_DB_CONNECTION,
    ERROR_DB_QUERY,
    ERROR_FILE_IO,
    ERROR_MEMORY,
    ERROR_INVALID_ARG
} ErrorCode;

typedef struct {
    ErrorCode code;
    char *message;
    char *details;
    const char *file;
    int line;
} Error;

Error *error_create(ErrorCode code, const char *message, const char *details);
void error_free(Error *err);
void error_print(const Error *err);
const char *error_code_to_string(ErrorCode code);

/* Logging */
typedef enum {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} LogLevel;

void log_init(const char *filename, LogLevel min_level);
void log_message(LogLevel level, const char *format, ...);
void log_debug(const char *format, ...);
void log_info(const char *format, ...);
void log_warn(const char *format, ...);
void log_error(const char *format, ...);
void log_shutdown(void);

/* Hash table for fast lookups during comparison */
typedef struct HashTable HashTable;

HashTable *hash_table_create(int capacity);
void hash_table_destroy(HashTable *ht);
void hash_table_insert(HashTable *ht, const char *key, void *value);
void *hash_table_get(HashTable *ht, const char *key);
bool hash_table_contains(HashTable *ht, const char *key);
void hash_table_remove(HashTable *ht, const char *key);
int hash_table_size(HashTable *ht);

/* File I/O utilities */
char *read_file_to_string(const char *filename);
bool write_string_to_file(const char *filename, const char *content);
char **read_directory_files(const char *dir_path, const char *extension, int *file_count);

#endif /* UTILS_H */
