#include "utils.h"
#include <stdlib.h>
#include <string.h>

/* Hash table entry */
typedef struct HashEntry {
    char *key;
    void *value;
    struct HashEntry *next;
} HashEntry;

/* Hash table structure */
struct HashTable {
    HashEntry **buckets;
    int capacity;
    int size;
};

/* Simple hash function (djb2) */
static unsigned long hash_string(const char *str) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}

/* Create hash table */
HashTable *hash_table_create(int capacity) {
    if (capacity <= 0) {
        capacity = 16;
    }

    HashTable *ht = malloc(sizeof(HashTable));
    if (!ht) {
        return NULL;
    }

    ht->buckets = calloc(capacity, sizeof(HashEntry *));
    if (!ht->buckets) {
        free(ht);
        return NULL;
    }

    ht->capacity = capacity;
    ht->size = 0;

    return ht;
}

/* Destroy hash table */
void hash_table_destroy(HashTable *ht) {
    if (!ht) {
        return;
    }

    for (int i = 0; i < ht->capacity; i++) {
        HashEntry *entry = ht->buckets[i];
        while (entry) {
            HashEntry *next = entry->next;
            free(entry->key);
            free(entry);
            entry = next;
        }
    }

    free(ht->buckets);
    free(ht);
}

/* Insert key-value pair */
void hash_table_insert(HashTable *ht, const char *key, void *value) {
    if (!ht || !key) {
        return;
    }

    unsigned long hash = hash_string(key);
    int index = hash % ht->capacity;

    /* Check if key already exists */
    HashEntry *entry = ht->buckets[index];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            entry->value = value;
            return;
        }
        entry = entry->next;
    }

    /* Create new entry */
    HashEntry *new_entry = malloc(sizeof(HashEntry));
    if (!new_entry) {
        return;
    }

    new_entry->key = strdup(key);
    new_entry->value = value;
    new_entry->next = ht->buckets[index];
    ht->buckets[index] = new_entry;
    ht->size++;
}

/* Get value by key */
void *hash_table_get(HashTable *ht, const char *key) {
    if (!ht || !key) {
        return NULL;
    }

    unsigned long hash = hash_string(key);
    int index = hash % ht->capacity;

    HashEntry *entry = ht->buckets[index];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }

    return NULL;
}

/* Check if key exists */
bool hash_table_contains(HashTable *ht, const char *key) {
    return hash_table_get(ht, key) != NULL;
}

/* Remove key-value pair */
void hash_table_remove(HashTable *ht, const char *key) {
    if (!ht || !key) {
        return;
    }

    unsigned long hash = hash_string(key);
    int index = hash % ht->capacity;

    HashEntry *prev = NULL;
    HashEntry *entry = ht->buckets[index];

    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            if (prev) {
                prev->next = entry->next;
            } else {
                ht->buckets[index] = entry->next;
            }

            free(entry->key);
            free(entry);
            ht->size--;
            return;
        }

        prev = entry;
        entry = entry->next;
    }
}

/* Get hash table size */
int hash_table_size(HashTable *ht) {
    return ht ? ht->size : 0;
}
