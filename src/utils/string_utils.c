#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

/* Trim whitespace from string */
char *str_trim(const char *str) {
    if (!str) {
        return NULL;
    }

    /* Skip leading whitespace */
    while (isspace(*str)) {
        str++;
    }

    if (*str == '\0') {
        return strdup("");
    }

    /* Find end of string */
    const char *end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) {
        end--;
    }

    /* Allocate trimmed string */
    size_t len = end - str + 1;
    char *trimmed = malloc(len + 1);
    if (!trimmed) {
        return NULL;
    }

    memcpy(trimmed, str, len);
    trimmed[len] = '\0';
    return trimmed;
}

/* Remove all whitespace from string */
char *str_remove_whitespace(const char *str) {
    if (!str) {
        return NULL;
    }

    size_t len = strlen(str);
    char *result = malloc(len + 1);
    if (!result) {
        return NULL;
    }

    char *dst = result;
    for (const char *src = str; *src; src++) {
        if (!isspace(*src)) {
            *dst++ = *src;
        }
    }
    *dst = '\0';

    return result;
}

/* Convert string to uppercase */
char *str_to_upper(const char *str) {
    if (!str) {
        return NULL;
    }

    size_t len = strlen(str);
    char *upper = malloc(len + 1);
    if (!upper) {
        return NULL;
    }

    for (size_t i = 0; i < len; i++) {
        upper[i] = toupper(str[i]);
    }
    upper[len] = '\0';

    return upper;
}

/* Convert string to lowercase */
char *str_to_lower(const char *str) {
    if (!str) {
        return NULL;
    }

    size_t len = strlen(str);
    char *lower = malloc(len + 1);
    if (!lower) {
        return NULL;
    }

    for (size_t i = 0; i < len; i++) {
        lower[i] = tolower(str[i]);
    }
    lower[len] = '\0';

    return lower;
}

/* Case-insensitive string comparison */
bool str_equals_ignore_case(const char *s1, const char *s2) {
    if (!s1 || !s2) {
        return s1 == s2;
    }

    while (*s1 && *s2) {
        if (tolower(*s1) != tolower(*s2)) {
            return false;
        }
        s1++;
        s2++;
    }

    return *s1 == *s2;
}

/* Concatenate two strings */
char *str_concat(const char *s1, const char *s2) {
    if (!s1) {
        return s2 ? strdup(s2) : NULL;
    }
    if (!s2) {
        return strdup(s1);
    }

    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);
    char *result = malloc(len1 + len2 + 1);
    if (!result) {
        return NULL;
    }

    memcpy(result, s1, len1);
    memcpy(result + len1, s2, len2);
    result[len1 + len2] = '\0';

    return result;
}

/* Split string by delimiter */
char **str_split(const char *str, char delimiter, int *count) {
    if (!str || !count) {
        if (count) *count = 0;
        return NULL;
    }

    /* Count delimiters */
    int num_parts = 1;
    for (const char *p = str; *p; p++) {
        if (*p == delimiter) {
            num_parts++;
        }
    }

    /* Allocate array */
    char **parts = malloc(sizeof(char *) * num_parts);
    if (!parts) {
        *count = 0;
        return NULL;
    }

    /* Split string */
    int idx = 0;
    const char *start = str;
    const char *end = str;

    while (*end) {
        if (*end == delimiter) {
            size_t len = end - start;
            parts[idx] = malloc(len + 1);
            if (parts[idx]) {
                memcpy(parts[idx], start, len);
                parts[idx][len] = '\0';
                idx++;
            }
            start = end + 1;
        }
        end++;
    }

    /* Add last part */
    size_t len = end - start;
    parts[idx] = malloc(len + 1);
    if (parts[idx]) {
        memcpy(parts[idx], start, len);
        parts[idx][len] = '\0';
        idx++;
    }

    *count = idx;
    return parts;
}

/* Free string array */
void str_free_array(char **arr, int count) {
    if (!arr) {
        return;
    }

    for (int i = 0; i < count; i++) {
        free(arr[i]);
    }
    free(arr);
}

/* String builder implementation */
struct StringBuilder {
    char *buffer;
    size_t capacity;
    size_t length;
};

StringBuilder *sb_create(void) {
    StringBuilder *sb = malloc(sizeof(StringBuilder));
    if (!sb) {
        return NULL;
    }

    sb->capacity = 256;
    sb->buffer = malloc(sb->capacity);
    if (!sb->buffer) {
        free(sb);
        return NULL;
    }

    sb->buffer[0] = '\0';
    sb->length = 0;

    return sb;
}

static void sb_ensure_capacity(StringBuilder *sb, size_t additional) {
    size_t required = sb->length + additional + 1;
    if (required <= sb->capacity) {
        return;
    }

    while (sb->capacity < required) {
        sb->capacity *= 2;
    }

    char *new_buffer = realloc(sb->buffer, sb->capacity);
    if (new_buffer) {
        sb->buffer = new_buffer;
    }
}

void sb_append(StringBuilder *sb, const char *str) {
    if (!sb || !str) {
        return;
    }

    size_t len = strlen(str);
    sb_ensure_capacity(sb, len);

    memcpy(sb->buffer + sb->length, str, len);
    sb->length += len;
    sb->buffer[sb->length] = '\0';
}

void sb_append_char(StringBuilder *sb, char c) {
    if (!sb) {
        return;
    }

    sb_ensure_capacity(sb, 1);
    sb->buffer[sb->length++] = c;
    sb->buffer[sb->length] = '\0';
}

void sb_append_fmt(StringBuilder *sb, const char *format, ...) {
    if (!sb || !format) {
        return;
    }

    va_list args;
    va_start(args, format);

    /* Calculate required size */
    va_list args_copy;
    va_copy(args_copy, args);
    int len = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);

    if (len < 0) {
        va_end(args);
        return;
    }

    sb_ensure_capacity(sb, len);
    vsnprintf(sb->buffer + sb->length, len + 1, format, args);
    sb->length += len;

    va_end(args);
}

char *sb_to_string(StringBuilder *sb) {
    if (!sb) {
        return NULL;
    }

    return strdup(sb->buffer);
}

void sb_free(StringBuilder *sb) {
    if (!sb) {
        return;
    }

    free(sb->buffer);
    free(sb);
}
