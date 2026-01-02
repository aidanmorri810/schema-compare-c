# Schema Compare Test Suite

This directory contains the test suite for the schema-compare project.

## Quick Start

```bash
# Run all tests
make test

# Clean and rebuild tests
make clean && make test

# Run specific test suite
./bin/test-runner --suite memory

# Run specific test
./bin/test-runner --suite memory --test alloc_basic

# List available test suites
./bin/test-runner --list

# Show help
./bin/test-runner --help
```

## Test Structure

```
tests/
├── test_framework.h      # Test framework macros and utilities
├── test_framework.c      # Test framework implementation
├── test_runner.c         # Main test runner executable
├── unit/                 # Unit tests
│   ├── test_memory.c
│   ├── test_hash_table.c
│   ├── test_lexer.c      # TODO
│   ├── test_parser.c     # TODO
│   └── ...
├── integration/          # Integration tests
│   ├── test_parser_integration.c    # TODO
│   ├── test_db_reader.c             # TODO
│   └── ...
├── fixtures/             # Test data
│   ├── ddl/              # SQL DDL files for testing
│   └── expected/         # Expected output files
└── data/                 # Real-world test data
    └── silka/            # Sakila DVD rental database
```

## Test Framework

The test framework is a lightweight custom framework with the following features:

### Assertion Macros

```c
ASSERT_TRUE(expr)              // Assert expression is true
ASSERT_FALSE(expr)             // Assert expression is false
ASSERT_EQ(a, b)                // Assert a == b
ASSERT_NEQ(a, b)               // Assert a != b
ASSERT_STR_EQ(a, b)            // Assert strings are equal
ASSERT_STR_NEQ(a, b)           // Assert strings are not equal
ASSERT_NULL(ptr)               // Assert pointer is NULL
ASSERT_NOT_NULL(ptr)           // Assert pointer is not NULL
ASSERT_PTR_EQ(a, b)            // Assert pointers are equal
ASSERT_FLOAT_EQ(a, b, epsilon) // Assert floats are approximately equal
```

### Writing Tests

```c
#include "../test_framework.h"
#include "your_module.h"

/* Test: Description of what this tests */
TEST_CASE(suite_name, test_name) {
    /* Setup */
    YourStruct *obj = create_object();
    ASSERT_NOT_NULL(obj);

    /* Test operations */
    int result = your_function(obj);
    ASSERT_EQ(result, EXPECTED_VALUE);

    /* Cleanup */
    destroy_object(obj);

    TEST_PASS();
}

/* Test suite definition */
static TestCase suite_tests[] = {
    {"test_name", test_suite_name_test_name, "suite_name"},
    /* Add more tests here */
};

void run_suite_tests(void) {
    run_test_suite("suite_name", NULL, NULL, suite_tests,
                   sizeof(suite_tests) / sizeof(suite_tests[0]));
}
```

### Test Results

- ✓ PASS (green) - Test passed
- ✗ FAIL (red) - Test failed
- ⊘ SKIP (yellow) - Test skipped

## Current Test Coverage

### Unit Tests (Completed)

- ✅ **Memory Management** (8 tests)
  - Context creation/destruction
  - Basic allocation
  - Multiple allocations
  - Large allocations
  - CreateTableStmt allocation
  - String duplication

- ✅ **Hash Table** (10 tests)
  - Create/destroy
  - Insert and get
  - Collision handling
  - Key overwriting
  - Case sensitivity
  - Empty and long keys

### Unit Tests (TODO)

- ⏳ **Lexer** - Tokenization tests
- ⏳ **Parser** - SQL parsing tests
- ⏳ **String Builder** - String concatenation tests
- ⏳ **Comparison** - Schema diff tests
- ⏳ **Report Generation** - Output formatting tests
- ⏳ **SQL Generator** - Migration script tests

### Integration Tests (TODO)

- ⏳ **Parser Integration** - Parse real-world DDL
- ⏳ **Database Reader** - Live database introspection
- ⏳ **End-to-End** - Complete workflow tests

## Test Data

### Sakila Database

The `tests/data/silka/` directory contains the Sakila sample database schema:
- 15 tables with real-world complexity
- Foreign keys, constraints, indexes
- Various data types and features

This is used for integration testing and parser validation.

### Test Fixtures

Create test fixtures in `tests/fixtures/ddl/`:
- `simple_table.sql` - Basic table definitions
- `complex_table.sql` - Tables with many features
- `edge_cases/` - Edge case scenarios

## Memory Testing

### Valgrind

Run tests with Valgrind to detect memory leaks:

```bash
make test VALGRIND=1
```

Or manually:

```bash
valgrind --leak-check=full --show-leak-kinds=all ./bin/test-runner
```

### Expected Results

All tests should run with:
- Zero memory leaks
- Zero invalid memory accesses
- All allocations freed

## Code Coverage

Generate code coverage reports:

```bash
make coverage
```

This creates an HTML report in `coverage_html/index.html`.

**Coverage Goals:**
- Overall: > 90%
- Critical paths: 100%
- Error handling: > 80%

## Continuous Integration

The project uses GitHub Actions for CI. See `.github/workflows/test.yml`.

Tests run automatically on:
- Every push
- Every pull request

CI runs:
- All unit tests
- All integration tests (with PostgreSQL)
- Valgrind memory checks
- Code coverage analysis

## Adding New Tests

1. Create test file in appropriate directory:
   - `tests/unit/` for unit tests
   - `tests/integration/` for integration tests

2. Include test framework:
   ```c
   #include "../test_framework.h"
   ```

3. Write test functions using `TEST_CASE` macro

4. Create test suite array and runner function

5. Add runner function call to `test_runner.c`:
   ```c
   void run_your_tests(void);  // Forward declaration

   int main() {
       // ...
       run_your_tests();  // Add this call
       // ...
   }
   ```

6. Update test list in `list_test_suites()` function

## Best Practices

### Test Independence
- Each test should be independent
- Tests should not depend on execution order
- Clean up all resources in each test

### Test Naming
- Use descriptive names: `test_suite_specific_behavior`
- Test one thing per test function
- Group related tests in the same suite

### Assertions
- Use specific assertions (ASSERT_EQ vs ASSERT_TRUE)
- Include helpful error messages
- Test both success and failure cases

### Memory Management
- Always create memory context in tests
- Always destroy context before returning
- Use Valgrind to verify no leaks

### Documentation
- Add comments explaining what each test does
- Document any non-obvious test data
- Update this README when adding new suites

## Troubleshooting

### Test Failures

If tests fail:
1. Check the error message - includes file and line number
2. Run specific test: `./bin/test-runner --suite X --test Y`
3. Add debugging output
4. Run with gdb: `gdb ./bin/test-runner`

### Compilation Errors

If tests don't compile:
1. Ensure all headers are included
2. Check that test file is in correct directory
3. Run `make clean && make test`
4. Check Makefile includes your test file

### Memory Leaks

If Valgrind reports leaks:
1. Find the test that's leaking
2. Check all allocations have matching free/destroy
3. Ensure memory contexts are destroyed
4. Check for early returns without cleanup

## Future Enhancements

Planned improvements to the test suite:

1. **Property-Based Testing**
   - Generate random valid SQL
   - Test with randomized inputs

2. **Fuzzing**
   - Use AFL or libFuzzer
   - Find parser edge cases

3. **Performance Benchmarks**
   - Track parsing speed
   - Monitor memory usage
   - Compare against previous versions

4. **Mutation Testing**
   - Verify test quality
   - Ensure tests catch bugs

## References

- See [TESTING_PLAN.md](../TESTING_PLAN.md) for comprehensive testing strategy
- PostgreSQL documentation: https://www.postgresql.org/docs/
- Valgrind manual: https://valgrind.org/docs/manual/
