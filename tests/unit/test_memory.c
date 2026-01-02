#include "../test_framework.h"
#include "sc_memory.h"
#include <string.h>

/* Test: Create and destroy memory context */
TEST_CASE(memory, context_create_destroy) {
    MemoryContext *ctx = memory_context_create("test_context");
    ASSERT_NOT_NULL(ctx);
    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test: Basic allocation */
TEST_CASE(memory, alloc_basic) {
    MemoryContext *ctx = memory_context_create("test_alloc");
    ASSERT_NOT_NULL(ctx);

    void *ptr = mem_alloc(ctx, 100);
    ASSERT_NOT_NULL(ptr);

    /* Write to the memory to ensure it's valid */
    memset(ptr, 0xAA, 100);

    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test: Zero-size allocation */
TEST_CASE(memory, alloc_zero_size) {
    MemoryContext *ctx = memory_context_create("test_zero");
    ASSERT_NOT_NULL(ctx);

    /* Zero-size allocation should return NULL or valid pointer */
    /* Implementation-dependent behavior - we just test it doesn't crash */
    (void)mem_alloc(ctx, 0);

    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test: Multiple allocations */
TEST_CASE(memory, multiple_allocations) {
    MemoryContext *ctx = memory_context_create("test_multiple");
    ASSERT_NOT_NULL(ctx);

    void *ptr1 = mem_alloc(ctx, 50);
    void *ptr2 = mem_alloc(ctx, 100);
    void *ptr3 = mem_alloc(ctx, 200);

    ASSERT_NOT_NULL(ptr1);
    ASSERT_NOT_NULL(ptr2);
    ASSERT_NOT_NULL(ptr3);

    /* Ensure pointers are different */
    ASSERT_NEQ((long)ptr1, (long)ptr2);
    ASSERT_NEQ((long)ptr2, (long)ptr3);
    ASSERT_NEQ((long)ptr1, (long)ptr3);

    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test: CreateTableStmt allocation */
TEST_CASE(memory, create_table_stmt_alloc) {
    MemoryContext *ctx = memory_context_create("test_stmt");
    ASSERT_NOT_NULL(ctx);

    CreateTableStmt *stmt = create_table_stmt_alloc(ctx);
    ASSERT_NOT_NULL(stmt);

    /* Verify key fields are initialized to NULL/0 */
    ASSERT_NULL(stmt->table_name);
    ASSERT_FALSE(stmt->if_not_exists);
    ASSERT_EQ(stmt->table_type, TABLE_TYPE_NORMAL);
    ASSERT_NULL(stmt->tablespace_name);

    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test: Large allocation */
TEST_CASE(memory, large_allocation) {
    MemoryContext *ctx = memory_context_create("test_large");
    ASSERT_NOT_NULL(ctx);

    /* Allocate 1MB */
    size_t large_size = 1024 * 1024;
    void *ptr = mem_alloc(ctx, large_size);
    ASSERT_NOT_NULL(ptr);

    /* Write pattern to memory */
    memset(ptr, 0x55, large_size);

    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test: Many small allocations */
TEST_CASE(memory, many_small_allocations) {
    MemoryContext *ctx = memory_context_create("test_many");
    ASSERT_NOT_NULL(ctx);

    #define NUM_ALLOCS 1000
    void *ptrs[NUM_ALLOCS];

    for (int i = 0; i < NUM_ALLOCS; i++) {
        ptrs[i] = mem_alloc(ctx, 10);
        ASSERT_NOT_NULL(ptrs[i]);
    }

    /* Verify all pointers are unique */
    for (int i = 0; i < NUM_ALLOCS - 1; i++) {
        for (int j = i + 1; j < NUM_ALLOCS; j++) {
            ASSERT_NEQ((long)ptrs[i], (long)ptrs[j]);
        }
    }

    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test: String duplication */
TEST_CASE(memory, strdup) {
    MemoryContext *ctx = memory_context_create("test_strdup");
    ASSERT_NOT_NULL(ctx);

    const char *original = "Hello, World!";
    char *copy = mem_strdup(ctx, original);

    ASSERT_NOT_NULL(copy);
    ASSERT_STR_EQ(copy, original);
    ASSERT_NEQ((long)copy, (long)original);

    memory_context_destroy(ctx);
    TEST_PASS();
}

/* Test suite definition */
static TestCase memory_tests[] = {
    {"context_create_destroy", test_memory_context_create_destroy, "memory"},
    {"alloc_basic", test_memory_alloc_basic, "memory"},
    {"alloc_zero_size", test_memory_alloc_zero_size, "memory"},
    {"multiple_allocations", test_memory_multiple_allocations, "memory"},
    {"create_table_stmt_alloc", test_memory_create_table_stmt_alloc, "memory"},
    {"large_allocation", test_memory_large_allocation, "memory"},
    {"many_small_allocations", test_memory_many_small_allocations, "memory"},
    {"strdup", test_memory_strdup, "memory"},
};

void run_memory_tests(void) {
    run_test_suite("memory", NULL, NULL, memory_tests,
                   sizeof(memory_tests) / sizeof(memory_tests[0]));
}
