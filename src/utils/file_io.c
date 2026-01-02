#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

/* Read entire file to string */
char *read_file_to_string(const char *filename) {
    if (!filename) {
        return NULL;
    }

    FILE *file = fopen(filename, "r");
    if (!file) {
        return NULL;
    }

    /* Get file size */
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size < 0) {
        fclose(file);
        return NULL;
    }

    /* Allocate buffer */
    char *content = malloc(size + 1);
    if (!content) {
        fclose(file);
        return NULL;
    }

    /* Read file */
    size_t read_size = fread(content, 1, size, file);
    content[read_size] = '\0';

    fclose(file);
    return content;
}

/* Write string to file */
bool write_string_to_file(const char *filename, const char *content) {
    if (!filename || !content) {
        return false;
    }

    FILE *file = fopen(filename, "w");
    if (!file) {
        return false;
    }

    size_t len = strlen(content);
    size_t written = fwrite(content, 1, len, file);

    fclose(file);
    return written == len;
}

/* Check if string ends with suffix */
static bool str_ends_with(const char *str, const char *suffix) {
    if (!str || !suffix) {
        return false;
    }

    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);

    if (suffix_len > str_len) {
        return false;
    }

    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

/* Read directory files with extension filter */
char **read_directory_files(const char *dir_path, const char *extension, int *file_count) {
    if (!dir_path || !file_count) {
        if (file_count) *file_count = 0;
        return NULL;
    }

    DIR *dir = opendir(dir_path);
    if (!dir) {
        *file_count = 0;
        return NULL;
    }

    /* Count matching files */
    int count = 0;
    int capacity = 16;
    char **files = malloc(sizeof(char *) * capacity);
    if (!files) {
        closedir(dir);
        *file_count = 0;
        return NULL;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        /* Skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        /* Check if it's a regular file */
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) != 0 || !S_ISREG(st.st_mode)) {
            continue;
        }

        /* Check extension if provided */
        if (extension && !str_ends_with(entry->d_name, extension)) {
            continue;
        }

        /* Expand array if needed */
        if (count >= capacity) {
            capacity *= 2;
            char **new_files = realloc(files, sizeof(char *) * capacity);
            if (!new_files) {
                break;
            }
            files = new_files;
        }

        /* Add file path */
        files[count] = strdup(full_path);
        if (files[count]) {
            count++;
        }
    }

    closedir(dir);
    *file_count = count;
    return files;
}
