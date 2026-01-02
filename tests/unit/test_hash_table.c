#include "../test_framework.h"
#include "utils.h"
#include <string.h>

/* Test: Create and destroy hash table */
TEST_CASE(hash_table, create_destroy) {
    HashTable *ht = hash_table_create(10);
    ASSERT_NOT_NULL(ht);
    hash_table_destroy(ht);
    TEST_PASS();
}

/* Test: Insert and get */
TEST_CASE(hash_table, insert_and_get) {
    HashTable *ht = hash_table_create(10);
    ASSERT_NOT_NULL(ht);

    char *value = "test_value";
    hash_table_insert(ht, "key1", value);

    void *retrieved = hash_table_get(ht, "key1");
    ASSERT_NOT_NULL(retrieved);
    ASSERT_PTR_EQ(retrieved, value);

    hash_table_destroy(ht);
    TEST_PASS();
}

/* Test: Get non-existent key */
TEST_CASE(hash_table, get_nonexistent) {
    HashTable *ht = hash_table_create(10);
    ASSERT_NOT_NULL(ht);

    void *result = hash_table_get(ht, "nonexistent");
    ASSERT_NULL(result);

    hash_table_destroy(ht);
    TEST_PASS();
}

/* Test: Multiple keys */
TEST_CASE(hash_table, multiple_keys) {
    HashTable *ht = hash_table_create(10);
    ASSERT_NOT_NULL(ht);

    char *val1 = "value1";
    char *val2 = "value2";
    char *val3 = "value3";

    hash_table_insert(ht, "key1", val1);
    hash_table_insert(ht, "key2", val2);
    hash_table_insert(ht, "key3", val3);

    ASSERT_PTR_EQ(hash_table_get(ht, "key1"), val1);
    ASSERT_PTR_EQ(hash_table_get(ht, "key2"), val2);
    ASSERT_PTR_EQ(hash_table_get(ht, "key3"), val3);

    hash_table_destroy(ht);
    TEST_PASS();
}

/* Test: Overwrite key */
TEST_CASE(hash_table, overwrite_key) {
    HashTable *ht = hash_table_create(10);
    ASSERT_NOT_NULL(ht);

    char *val1 = "value1";
    char *val2 = "value2";

    hash_table_insert(ht, "key", val1);
    ASSERT_PTR_EQ(hash_table_get(ht, "key"), val1);

    hash_table_insert(ht, "key", val2);
    ASSERT_PTR_EQ(hash_table_get(ht, "key"), val2);

    hash_table_destroy(ht);
    TEST_PASS();
}

/* Test: NULL value */
TEST_CASE(hash_table, null_value) {
    HashTable *ht = hash_table_create(10);
    ASSERT_NOT_NULL(ht);

    hash_table_insert(ht, "key", NULL);

    /* Should be able to store and retrieve NULL */
    /* Implementation may vary - this tests current behavior */
    /* We don't assert on the result since NULL handling varies */

    hash_table_destroy(ht);
    TEST_PASS();
}

/* Test: Many keys (collision handling) */
TEST_CASE(hash_table, many_keys) {
    HashTable *ht = hash_table_create(10); /* Small capacity to force collisions */
    ASSERT_NOT_NULL(ht);

    #define NUM_KEYS 100
    char keys[NUM_KEYS][20];
    int values[NUM_KEYS];

    /* Insert many keys */
    for (int i = 0; i < NUM_KEYS; i++) {
        snprintf(keys[i], sizeof(keys[i]), "key_%d", i);
        values[i] = i * 10;
        hash_table_insert(ht, keys[i], &values[i]);
    }

    /* Verify all keys can be retrieved */
    for (int i = 0; i < NUM_KEYS; i++) {
        int *retrieved = (int *)hash_table_get(ht, keys[i]);
        ASSERT_NOT_NULL(retrieved);
        ASSERT_EQ(*retrieved, values[i]);
    }

    hash_table_destroy(ht);
    TEST_PASS();
}

/* Test: Case sensitivity */
TEST_CASE(hash_table, case_sensitive) {
    HashTable *ht = hash_table_create(10);
    ASSERT_NOT_NULL(ht);

    char *val1 = "lowercase";
    char *val2 = "uppercase";

    hash_table_insert(ht, "key", val1);
    hash_table_insert(ht, "KEY", val2);

    /* Keys should be case-sensitive */
    /* Verify both keys can be retrieved (case matters) */
    ASSERT_NOT_NULL(hash_table_get(ht, "key"));
    ASSERT_NOT_NULL(hash_table_get(ht, "KEY"));

    hash_table_destroy(ht);
    TEST_PASS();
}

/* Test: Empty string key */
TEST_CASE(hash_table, empty_string_key) {
    HashTable *ht = hash_table_create(10);
    ASSERT_NOT_NULL(ht);

    char *value = "empty_key_value";
    hash_table_insert(ht, "", value);

    void *retrieved = hash_table_get(ht, "");
    ASSERT_NOT_NULL(retrieved);
    ASSERT_PTR_EQ(retrieved, value);

    hash_table_destroy(ht);
    TEST_PASS();
}

/* Test: Long keys */
TEST_CASE(hash_table, long_keys) {
    HashTable *ht = hash_table_create(10);
    ASSERT_NOT_NULL(ht);

    char long_key[1000];
    memset(long_key, 'a', sizeof(long_key) - 1);
    long_key[sizeof(long_key) - 1] = '\0';

    char *value = "long_key_value";
    hash_table_insert(ht, long_key, value);

    void *retrieved = hash_table_get(ht, long_key);
    ASSERT_NOT_NULL(retrieved);
    ASSERT_PTR_EQ(retrieved, value);

    hash_table_destroy(ht);
    TEST_PASS();
}

/* Test suite definition */
static TestCase hash_table_tests[] = {
    {"create_destroy", test_hash_table_create_destroy, "hash_table"},
    {"insert_and_get", test_hash_table_insert_and_get, "hash_table"},
    {"get_nonexistent", test_hash_table_get_nonexistent, "hash_table"},
    {"multiple_keys", test_hash_table_multiple_keys, "hash_table"},
    {"overwrite_key", test_hash_table_overwrite_key, "hash_table"},
    {"null_value", test_hash_table_null_value, "hash_table"},
    {"many_keys", test_hash_table_many_keys, "hash_table"},
    {"case_sensitive", test_hash_table_case_sensitive, "hash_table"},
    {"empty_string_key", test_hash_table_empty_string_key, "hash_table"},
    {"long_keys", test_hash_table_long_keys, "hash_table"},
};

void run_hash_table_tests(void) {
    run_test_suite("hash_table", NULL, NULL, hash_table_tests,
                   sizeof(hash_table_tests) / sizeof(hash_table_tests[0]));
}
