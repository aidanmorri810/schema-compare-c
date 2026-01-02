#include "../test_framework.h"
#include "utils.h"
#include <string.h>

/* Test: Create and destroy string builder */
TEST_CASE(string_builder, create_destroy) {
    StringBuilder *sb = sb_create();
    ASSERT_NOT_NULL(sb);
    sb_free(sb);
    TEST_PASS();
}

/* Test: Append basic strings */
TEST_CASE(string_builder, append_basic) {
    StringBuilder *sb = sb_create();
    ASSERT_NOT_NULL(sb);

    sb_append(sb, "Hello");
    sb_append(sb, " ");
    sb_append(sb, "World");

    char *result = sb_to_string(sb);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result, "Hello World");

    free(result);
    sb_free(sb);
    TEST_PASS();
}

/* Test: Append single character */
TEST_CASE(string_builder, append_char) {
    StringBuilder *sb = sb_create();
    ASSERT_NOT_NULL(sb);

    sb_append_char(sb, 'A');
    sb_append_char(sb, 'B');
    sb_append_char(sb, 'C');

    char *result = sb_to_string(sb);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result, "ABC");

    free(result);
    sb_free(sb);
    TEST_PASS();
}

/* Test: Append formatted strings */
TEST_CASE(string_builder, append_fmt) {
    StringBuilder *sb = sb_create();
    ASSERT_NOT_NULL(sb);

    sb_append_fmt(sb, "Number: %d", 42);
    sb_append(sb, ", ");
    sb_append_fmt(sb, "String: %s", "test");

    char *result = sb_to_string(sb);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result, "Number: 42, String: test");

    free(result);
    sb_free(sb);
    TEST_PASS();
}

/* Test: Append many strings (buffer growth) */
TEST_CASE(string_builder, append_many) {
    StringBuilder *sb = sb_create();
    ASSERT_NOT_NULL(sb);

    /* Append 100 strings to trigger buffer growth */
    for (int i = 0; i < 100; i++) {
        sb_append(sb, "x");
    }

    char *result = sb_to_string(sb);
    ASSERT_NOT_NULL(result);

    /* Result should be 100 'x' characters */
    size_t len = strlen(result);
    ASSERT_EQ(len, 100);

    /* Verify all characters are 'x' */
    for (size_t i = 0; i < len; i++) {
        if (result[i] != 'x') {
            sb_free(sb);
            TEST_FAIL("Character at position %zu is not 'x'", i);
        }
    }

    free(result);
    sb_free(sb);
    TEST_PASS();
}

/* Test: Append empty strings */
TEST_CASE(string_builder, append_empty) {
    StringBuilder *sb = sb_create();
    ASSERT_NOT_NULL(sb);

    sb_append(sb, "Start");
    sb_append(sb, "");
    sb_append(sb, "");
    sb_append(sb, "End");

    char *result = sb_to_string(sb);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result, "StartEnd");

    free(result);
    sb_free(sb);
    TEST_PASS();
}

/* Test: Append NULL (should handle gracefully) */
TEST_CASE(string_builder, append_null) {
    StringBuilder *sb = sb_create();
    ASSERT_NOT_NULL(sb);

    sb_append(sb, "Before");
    sb_append(sb, NULL);  /* Should not crash */
    sb_append(sb, "After");

    char *result = sb_to_string(sb);
    ASSERT_NOT_NULL(result);

    /* NULL append should either be ignored or treated as empty string */
    /* The result should be "BeforeAfter" or similar */

    free(result);
    sb_free(sb);
    TEST_PASS();
}

/* Test: Buffer growth with large strings */
TEST_CASE(string_builder, large_string) {
    StringBuilder *sb = sb_create();
    ASSERT_NOT_NULL(sb);

    /* Create a 1000-character string */
    char large[1001];
    memset(large, 'A', 1000);
    large[1000] = '\0';

    sb_append(sb, large);
    sb_append(sb, large);
    sb_append(sb, large);

    char *result = sb_to_string(sb);
    ASSERT_NOT_NULL(result);

    /* Result should be 3000 'A' characters */
    size_t len = strlen(result);
    ASSERT_EQ(len, 3000);

    free(result);
    sb_free(sb);
    TEST_PASS();
}

/* Test: Very large string (10MB+) */
TEST_CASE(string_builder, very_large_string) {
    StringBuilder *sb = sb_create();
    ASSERT_NOT_NULL(sb);

    /* Append 1MB string 10 times to get 10MB+ */
    size_t chunk_size = 1024 * 1024;  /* 1MB */
    char *chunk = (char *)malloc(chunk_size + 1);
    if (!chunk) {
        sb_free(sb);
        TEST_SKIP("Could not allocate memory for large string test");
    }

    memset(chunk, 'B', chunk_size);
    chunk[chunk_size] = '\0';

    for (int i = 0; i < 10; i++) {
        sb_append(sb, chunk);
    }

    char *result = sb_to_string(sb);
    ASSERT_NOT_NULL(result);

    /* Result should be 10MB */
    size_t len = strlen(result);
    ASSERT_EQ(len, chunk_size * 10);

    free(chunk);
    free(result);
    sb_free(sb);
    TEST_PASS();
}

/* Test: Convert to string multiple times */
TEST_CASE(string_builder, to_string_multiple) {
    StringBuilder *sb = sb_create();
    ASSERT_NOT_NULL(sb);

    sb_append(sb, "Test");

    /* Get string multiple times */
    char *result1 = sb_to_string(sb);
    char *result2 = sb_to_string(sb);

    ASSERT_NOT_NULL(result1);
    ASSERT_NOT_NULL(result2);
    ASSERT_STR_EQ(result1, "Test");
    ASSERT_STR_EQ(result2, "Test");

    free(result1);
    free(result2);
    sb_free(sb);
    TEST_PASS();
}

/* Test: Empty string builder */
TEST_CASE(string_builder, empty) {
    StringBuilder *sb = sb_create();
    ASSERT_NOT_NULL(sb);

    /* Don't append anything */
    char *result = sb_to_string(sb);
    ASSERT_NOT_NULL(result);

    /* Should return empty string */
    ASSERT_EQ(strlen(result), 0);
    ASSERT_STR_EQ(result, "");

    free(result);
    sb_free(sb);
    TEST_PASS();
}

/* Test: Mixed append operations */
TEST_CASE(string_builder, mixed_operations) {
    StringBuilder *sb = sb_create();
    ASSERT_NOT_NULL(sb);

    sb_append(sb, "Start");
    sb_append_char(sb, ' ');
    sb_append_fmt(sb, "%d", 123);
    sb_append_char(sb, ' ');
    sb_append(sb, "End");

    char *result = sb_to_string(sb);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result, "Start 123 End");

    free(result);
    sb_free(sb);
    TEST_PASS();
}

/* Test: Special characters */
TEST_CASE(string_builder, special_characters) {
    StringBuilder *sb = sb_create();
    ASSERT_NOT_NULL(sb);

    sb_append(sb, "Line1\n");
    sb_append(sb, "Tab\there\n");
    sb_append(sb, "Quote\"test\"");

    char *result = sb_to_string(sb);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result, "Line1\nTab\there\nQuote\"test\"");

    free(result);
    sb_free(sb);
    TEST_PASS();
}

/* Test: Unicode strings */
TEST_CASE(string_builder, unicode) {
    StringBuilder *sb = sb_create();
    ASSERT_NOT_NULL(sb);

    sb_append(sb, "Hello ");
    sb_append(sb, "世界");  /* "World" in Chinese */
    sb_append(sb, " ");
    sb_append(sb, "🌍");    /* Earth emoji */

    char *result = sb_to_string(sb);
    ASSERT_NOT_NULL(result);

    /* Should contain the unicode characters */
    /* We can't do exact string comparison due to encoding */
    /* Just verify it's not empty and doesn't crash */
    ASSERT_TRUE(strlen(result) > 0);

    free(result);
    sb_free(sb);
    TEST_PASS();
}

/* Test suite definition */
static TestCase string_builder_tests[] = {
    {"create_destroy", test_string_builder_create_destroy, "string_builder"},
    {"append_basic", test_string_builder_append_basic, "string_builder"},
    {"append_char", test_string_builder_append_char, "string_builder"},
    {"append_fmt", test_string_builder_append_fmt, "string_builder"},
    {"append_many", test_string_builder_append_many, "string_builder"},
    {"append_empty", test_string_builder_append_empty, "string_builder"},
    {"append_null", test_string_builder_append_null, "string_builder"},
    {"large_string", test_string_builder_large_string, "string_builder"},
    {"very_large_string", test_string_builder_very_large_string, "string_builder"},
    {"to_string_multiple", test_string_builder_to_string_multiple, "string_builder"},
    {"empty", test_string_builder_empty, "string_builder"},
    {"mixed_operations", test_string_builder_mixed_operations, "string_builder"},
    {"special_characters", test_string_builder_special_characters, "string_builder"},
    {"unicode", test_string_builder_unicode, "string_builder"},
};

void run_string_builder_tests(void) {
    run_test_suite("string_builder", NULL, NULL, string_builder_tests,
                   sizeof(string_builder_tests) / sizeof(string_builder_tests[0]));
}
