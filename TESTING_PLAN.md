# Schema Compare Testing Plan

## Overview

This document outlines a comprehensive testing strategy for the schema-compare tool. The testing approach covers unit tests, integration tests, end-to-end tests, and validation tests using both synthetic and real-world PostgreSQL schemas.

## Testing Levels

### 1. Unit Tests (Per Module)
### 2. Integration Tests (Cross Module)
### 3. End-to-End Tests (Full Workflow)
### 4. Regression Tests (Edge Cases & Bug Fixes)
### 5. Performance Tests (Large Schemas)

---

## Test Infrastructure

### Test Framework
- **Simple C Test Framework**: Custom lightweight framework (no external dependencies)
- **Test Runner**: Single executable that runs all tests
- **Assertion Macros**: `ASSERT_TRUE`, `ASSERT_FALSE`, `ASSERT_EQ`, `ASSERT_STR_EQ`, `ASSERT_NULL`, `ASSERT_NOT_NULL`
- **Test Fixtures**: Setup/teardown functions for each test suite
- **Memory Leak Detection**: Valgrind integration for memory testing

### Test Organization
```
tests/
├── unit/
│   ├── test_lexer.c
│   ├── test_parser.c
│   ├── test_memory.c
│   ├── test_hash_table.c
│   ├── test_string_builder.c
│   ├── test_compare.c
│   ├── test_diff.c
│   ├── test_report.c
│   └── test_sql_generator.c
├── integration/
│   ├── test_parser_integration.c
│   ├── test_db_reader.c
│   ├── test_compare_workflow.c
│   └── test_end_to_end.c
├── fixtures/
│   ├── ddl/
│   │   ├── simple_table.sql
│   │   ├── complex_table.sql
│   │   ├── partitioned_table.sql
│   │   ├── inherited_table.sql
│   │   └── edge_cases/
│   └── expected/
│       ├── reports/
│       └── migrations/
├── data/
│   └── silka/          # Real-world test database (DVD rental)
└── test_runner.c        # Main test harness
```

---

## Phase 1: Unit Tests

### 1.1 Memory Management Tests (`test_memory.c`)

**Test Cases:**
- `test_context_create_destroy`: Create and destroy contexts without leaks
- `test_alloc_basic`: Allocate memory within context
- `test_alloc_zero_size`: Edge case for zero-size allocation
- `test_multiple_allocations`: Multiple allocations in same context
- `test_nested_contexts`: Parent/child context hierarchy
- `test_context_reset`: Reset context and verify all memory freed
- `test_create_table_stmt_alloc`: Allocate CreateTableStmt structure
- `test_memory_alignment`: Verify proper memory alignment
- `test_out_of_memory`: Handle allocation failures gracefully

**Metrics:**
- Zero memory leaks (Valgrind)
- Proper cleanup of all allocations

### 1.2 Hash Table Tests (`test_hash_table.c`)

**Test Cases:**
- `test_hash_table_create`: Basic creation and destruction
- `test_insert_and_get`: Insert keys and retrieve values
- `test_collision_handling`: Force hash collisions
- `test_overwrite_key`: Insert same key twice
- `test_null_value`: Store NULL values
- `test_many_keys`: Insert 1000+ keys
- `test_get_nonexistent`: Get non-existent key returns NULL
- `test_case_sensitivity`: Verify case-sensitive keys
- `test_empty_string_key`: Edge case for empty string
- `test_hash_distribution`: Verify even hash distribution

**Metrics:**
- O(1) average lookup time
- Proper collision handling

### 1.3 String Builder Tests (`test_string_builder.c`)

**Test Cases:**
- `test_sb_create_destroy`: Basic lifecycle
- `test_sb_append_basic`: Append simple strings
- `test_sb_append_many`: Append 100+ strings
- `test_sb_append_empty`: Append empty strings
- `test_sb_append_null`: Append NULL (should handle gracefully)
- `test_sb_buffer_growth`: Verify dynamic buffer resizing
- `test_sb_to_string`: Convert to final string
- `test_sb_large_string`: Append very large strings (10MB+)

**Metrics:**
- Correct concatenation
- No buffer overflows

### 1.4 Lexer Tests (`test_lexer.c`)

**Test Cases:**
- `test_tokenize_keywords`: All 130+ SQL keywords
- `test_tokenize_identifiers`: Simple and quoted identifiers
- `test_tokenize_strings`: String literals with escapes
- `test_tokenize_numbers`: Integer and decimal numbers
- `test_tokenize_operators`: All operators (=, >=, <=, etc.)
- `test_tokenize_comments`: Line and block comments
- `test_tokenize_whitespace`: Tab, space, newline handling
- `test_tokenize_mixed`: Complete SQL statement
- `test_line_column_tracking`: Verify accurate position tracking
- `test_unterminated_string`: Error handling
- `test_unterminated_comment`: Error handling
- `test_keyword_case_insensitive`: CREATE vs create vs CrEaTe
- `test_unicode_identifiers`: Unicode in identifiers

**Metrics:**
- 100% keyword recognition
- Accurate line/column tracking

### 1.5 Parser Tests (`test_parser.c`)

**Test Cases:**

**Basic Table Creation:**
- `test_parse_simple_table`: `CREATE TABLE t (id int)`
- `test_parse_if_not_exists`: `CREATE TABLE IF NOT EXISTS`
- `test_parse_temporary`: `CREATE TEMPORARY TABLE`
- `test_parse_unlogged`: `CREATE UNLOGGED TABLE`
- `test_parse_schema_qualified`: `CREATE TABLE myschema.mytable`

**Column Definitions:**
- `test_parse_column_int`: Integer columns
- `test_parse_column_varchar`: VARCHAR(n) columns
- `test_parse_column_numeric`: NUMERIC(p,s) columns
- `test_parse_column_text`: TEXT columns
- `test_parse_column_array`: Array types (text[])
- `test_parse_column_not_null`: NOT NULL constraint
- `test_parse_column_default`: DEFAULT expressions
- `test_parse_column_generated`: GENERATED ALWAYS AS IDENTITY
- `test_parse_column_storage`: STORAGE PLAIN/EXTERNAL/EXTENDED/MAIN
- `test_parse_column_compression`: COMPRESSION pglz/lz4
- `test_parse_column_collate`: COLLATE clause

**Constraints:**
- `test_parse_primary_key`: PRIMARY KEY
- `test_parse_foreign_key`: FOREIGN KEY with REFERENCES
- `test_parse_unique`: UNIQUE constraint
- `test_parse_check`: CHECK constraint
- `test_parse_constraint_deferrable`: DEFERRABLE constraints
- `test_parse_constraint_initially`: INITIALLY DEFERRED/IMMEDIATE
- `test_parse_constraint_not_enforced`: NOT ENFORCED
- `test_parse_fk_on_delete`: ON DELETE CASCADE/SET NULL/etc
- `test_parse_fk_on_update`: ON UPDATE CASCADE/SET NULL/etc
- `test_parse_fk_match`: MATCH FULL/PARTIAL/SIMPLE

**Advanced Features:**
- `test_parse_inherits`: INHERITS clause
- `test_parse_partition_by_range`: PARTITION BY RANGE
- `test_parse_partition_by_list`: PARTITION BY LIST
- `test_parse_partition_by_hash`: PARTITION BY HASH
- `test_parse_partition_of`: CREATE TABLE ... PARTITION OF
- `test_parse_of_type`: CREATE TABLE ... OF type_name
- `test_parse_like_clause`: LIKE other_table INCLUDING/EXCLUDING
- `test_parse_with_options`: WITH (fillfactor=70)
- `test_parse_tablespace`: TABLESPACE pg_default

**Error Recovery:**
- `test_parse_syntax_error`: Invalid syntax handling
- `test_parse_missing_semicolon`: Missing terminator
- `test_parse_unexpected_token`: Unexpected token
- `test_parse_incomplete_statement`: Truncated SQL

**Metrics:**
- Parse all valid PostgreSQL CREATE TABLE variants
- Accurate error reporting with line/column

### 1.6 Comparison Tests (`test_compare.c`)

**Test Cases:**
- `test_compare_identical`: No differences
- `test_compare_table_added`: Detect new table
- `test_compare_table_removed`: Detect deleted table
- `test_compare_column_added`: New column detected
- `test_compare_column_removed`: Deleted column detected
- `test_compare_column_type_changed`: Type change (int → bigint)
- `test_compare_column_null_changed`: NOT NULL added/removed
- `test_compare_column_default_changed`: DEFAULT value changed
- `test_compare_constraint_added`: New constraint
- `test_compare_constraint_removed`: Deleted constraint
- `test_compare_type_normalization`: int4 == integer, bool == boolean
- `test_compare_case_insensitive`: Case-insensitive mode
- `test_compare_ignore_tablespace`: Option to ignore tablespaces

**Metrics:**
- 100% detection of schema differences
- Correct severity assignment

### 1.7 Diff Tests (`test_diff.c`)

**Test Cases:**
- `test_diff_create`: Create diff entries
- `test_diff_severity_critical`: Verify critical severity
- `test_diff_severity_warning`: Verify warning severity
- `test_diff_severity_info`: Verify info severity
- `test_diff_to_string`: Human-readable diff description
- `test_diff_list_append`: Build diff list
- `test_diff_filter_by_severity`: Filter diffs by severity

### 1.8 Report Generation Tests (`test_report.c`)

**Test Cases:**
- `test_report_no_diff`: Report with no differences
- `test_report_text_format`: Plain text output
- `test_report_markdown_format`: Markdown output
- `test_report_json_format`: JSON output
- `test_report_with_color`: ANSI color codes
- `test_report_severity_icons`: ✓ ⚠ ✗ icons
- `test_report_verbosity_summary`: Summary level output
- `test_report_verbosity_detailed`: Detailed level output
- `test_report_line_wrapping`: Respect max_width

**Metrics:**
- Valid output format
- Readable formatting

### 1.9 SQL Generator Tests (`test_sql_generator.c`)

**Test Cases:**
- `test_gen_add_column`: ALTER TABLE ADD COLUMN
- `test_gen_drop_column`: ALTER TABLE DROP COLUMN
- `test_gen_alter_column_type`: ALTER COLUMN TYPE
- `test_gen_set_not_null`: ALTER COLUMN SET NOT NULL
- `test_gen_drop_not_null`: ALTER COLUMN DROP NOT NULL
- `test_gen_set_default`: ALTER COLUMN SET DEFAULT
- `test_gen_add_constraint`: ALTER TABLE ADD CONSTRAINT
- `test_gen_drop_constraint`: ALTER TABLE DROP CONSTRAINT
- `test_gen_identifier_quoting`: Proper "quoting" of identifiers
- `test_gen_transaction_wrapping`: BEGIN/COMMIT wrapping
- `test_gen_rollback_sql`: Reverse migration SQL
- `test_gen_destructive_warnings`: Comments for DROP operations
- `test_gen_if_exists`: IF EXISTS for safe operations

**Metrics:**
- Valid PostgreSQL SQL
- Idempotent migrations

---

## Phase 2: Integration Tests

### 2.1 Parser Integration (`test_parser_integration.c`)

**Test Cases:**
- `test_parse_real_world_table`: Parse actual Sakila tables
- `test_parse_all_sakila_tables`: Parse all 15 Sakila tables
- `test_parse_complex_constraints`: Multi-column FK, complex CHECK
- `test_parse_large_file`: 10,000+ line SQL file
- `test_parse_with_comments`: SQL with extensive comments
- `test_parse_and_reconstruct`: Parse → generate SQL → re-parse

**Test Data:**
- Use `tests/data/silka/tables/*.sql` files

### 2.2 Database Reader Integration (`test_db_reader.c`)

**Prerequisites:**
- PostgreSQL test database running
- Test credentials configured

**Test Cases:**
- `test_db_connect`: Connect to test database
- `test_db_read_table`: Read single table schema
- `test_db_read_schema`: Read all tables in schema
- `test_db_introspect_columns`: Column metadata accuracy
- `test_db_introspect_constraints`: Constraint metadata accuracy
- `test_db_introspect_partitioned`: Partitioned table detection
- `test_db_introspect_inherited`: Inheritance hierarchy
- `test_db_connection_failure`: Handle connection errors
- `test_db_invalid_table`: Non-existent table handling

**Setup Script:**
```sql
-- Create test database
CREATE DATABASE schema_compare_test;
\c schema_compare_test

-- Load Sakila schema
\i tests/data/silka/types/*.sql
\i tests/data/silka/tables/*.sql
```

### 2.3 Compare Workflow Integration (`test_compare_workflow.c`)

**Test Cases:**
- `test_workflow_file_to_file`: Compare two DDL files
- `test_workflow_file_to_db`: Compare DDL file to live database
- `test_workflow_db_to_db`: Compare two database schemas
- `test_workflow_multi_table`: Compare 10+ tables at once
- `test_workflow_with_options`: Test various compare options
- `test_workflow_generate_report`: Full report generation
- `test_workflow_generate_migration`: Full migration generation

---

## Phase 3: End-to-End Tests

### 3.1 Complete Application Tests (`test_end_to_end.c`)

**Test Cases:**
- `test_e2e_simple_diff`: Simple file comparison
- `test_e2e_database_diff`: Database comparison
- `test_e2e_report_output`: Report written to file
- `test_e2e_sql_output`: Migration SQL written to file
- `test_e2e_no_differences`: Handle identical schemas
- `test_e2e_invalid_input`: Error handling
- `test_e2e_help_output`: --help flag
- `test_e2e_version_output`: --version flag

**Test Script:**
```bash
#!/bin/bash
# Run end-to-end tests

# Test 1: Compare two files
./bin/schema-compare \
  tests/fixtures/ddl/v1_schema.sql \
  tests/fixtures/ddl/v2_schema.sql \
  --output /tmp/report.txt

# Test 2: Generate migration
./bin/schema-compare \
  tests/fixtures/ddl/v1_schema.sql \
  tests/fixtures/ddl/v2_schema.sql \
  --sql /tmp/migration.sql

# Test 3: Database comparison
./bin/schema-compare \
  --source-db "host=localhost dbname=test1" \
  --target-db "host=localhost dbname=test2" \
  --output /tmp/db_report.txt
```

---

## Phase 4: Regression Tests

### 4.1 Edge Cases

**Test Fixtures:** `tests/fixtures/ddl/edge_cases/`

**Cases:**
- `empty_table.sql`: Table with no columns
- `single_column.sql`: Minimal table
- `very_long_identifier.sql`: 63-character identifiers
- `unicode_identifiers.sql`: UTF-8 in names
- `quoted_keywords.sql`: Keywords as column names ("select", "from")
- `array_of_arrays.sql`: int[][] types
- `complex_default.sql`: DEFAULT with expressions
- `complex_check.sql`: CHECK with subqueries
- `multicolumn_pk.sql`: Composite primary keys
- `circular_fk.sql`: Circular foreign key references
- `all_constraints.sql`: All constraint types on one table
- `partition_levels.sql`: Multi-level partitioning
- `inheritance_chain.sql`: Multi-level inheritance
- `storage_parameters.sql`: All WITH options
- `all_data_types.sql`: Every PostgreSQL type

### 4.2 Known Bug Fixes

**Maintain regression tests for all fixed bugs:**
- `test_bug_001_expression_field`: Expression vs expr_string field
- `test_bug_002_constraint_type`: GENERATED_AS vs GENERATED_ALWAYS
- `test_bug_003_hash_functions`: insert vs set naming
- Future bugs...

---

## Phase 5: Performance Tests

### 5.1 Scalability Tests

**Test Cases:**
- `test_perf_100_tables`: Compare 100 tables
- `test_perf_1000_tables`: Compare 1,000 tables
- `test_perf_large_table`: Table with 500 columns
- `test_perf_deep_nesting`: Deep partition hierarchy
- `test_perf_memory_usage`: Memory usage profiling
- `test_perf_parse_speed`: Parse 10,000 lines/second

**Metrics:**
- Parsing speed: > 10,000 lines/second
- Memory usage: < 100MB for 1,000 tables
- Comparison time: < 1 second for 100 tables

### 5.2 Memory Leak Tests

**Run all tests under Valgrind:**
```bash
make test VALGRIND=1
```

**Requirements:**
- Zero memory leaks
- Zero invalid memory accesses
- All allocations freed

---

## Phase 6: Test Fixtures

### 6.1 Simple Test Cases

**tests/fixtures/ddl/simple_table.sql**
```sql
CREATE TABLE users (
    id integer PRIMARY KEY,
    username varchar(50) NOT NULL,
    email varchar(100) UNIQUE,
    created_at timestamp DEFAULT now()
);
```

**tests/fixtures/ddl/simple_table_v2.sql** (modified version)
```sql
CREATE TABLE users (
    id bigint PRIMARY KEY,  -- Changed type
    username varchar(100) NOT NULL,  -- Increased length
    email varchar(100) UNIQUE,
    created_at timestamp DEFAULT now(),
    updated_at timestamp  -- Added column
);
```

### 6.2 Complex Test Cases

**tests/fixtures/ddl/complex_table.sql**
```sql
CREATE TABLE orders (
    order_id integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    customer_id integer NOT NULL,
    order_date timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
    status varchar(20) NOT NULL DEFAULT 'pending',
    total_amount numeric(10,2) NOT NULL,
    notes text,
    metadata jsonb,
    CONSTRAINT orders_customer_id_fkey
        FOREIGN KEY (customer_id)
        REFERENCES customers(customer_id)
        ON DELETE CASCADE
        ON UPDATE RESTRICT,
    CONSTRAINT orders_status_check
        CHECK (status IN ('pending', 'processing', 'shipped', 'delivered', 'cancelled')),
    CONSTRAINT orders_total_positive
        CHECK (total_amount >= 0)
) WITH (fillfactor=80);

CREATE INDEX idx_orders_customer ON orders(customer_id);
CREATE INDEX idx_orders_date ON orders(order_date DESC);
```

### 6.3 Partitioned Table Tests

**tests/fixtures/ddl/partitioned_table.sql**
```sql
CREATE TABLE events (
    event_id bigint NOT NULL,
    event_type varchar(50) NOT NULL,
    event_date date NOT NULL,
    data jsonb
) PARTITION BY RANGE (event_date);

CREATE TABLE events_2024 PARTITION OF events
    FOR VALUES FROM ('2024-01-01') TO ('2025-01-01');

CREATE TABLE events_2025 PARTITION OF events
    FOR VALUES FROM ('2025-01-01') TO ('2026-01-01');
```

### 6.4 Inheritance Tests

**tests/fixtures/ddl/inherited_table.sql**
```sql
CREATE TABLE base_entity (
    id integer PRIMARY KEY,
    created_at timestamp NOT NULL DEFAULT now(),
    updated_at timestamp NOT NULL DEFAULT now()
);

CREATE TABLE users (
    username varchar(50) NOT NULL UNIQUE,
    email varchar(100) NOT NULL
) INHERITS (base_entity);

CREATE TABLE products (
    name varchar(100) NOT NULL,
    price numeric(10,2) NOT NULL
) INHERITS (base_entity);
```

---

## Test Execution Plan

### Test Priority Order

1. **Unit Tests (Core)**: Memory, Hash Table, String Builder
2. **Unit Tests (Parser)**: Lexer, Parser
3. **Unit Tests (Comparison)**: Compare, Diff
4. **Unit Tests (Output)**: Report, SQL Generator
5. **Integration Tests**: Parser Integration, DB Reader
6. **End-to-End Tests**: Full workflow
7. **Performance Tests**: Scalability
8. **Regression Tests**: Edge cases

### Continuous Integration

**CI Pipeline (.github/workflows/test.yml):**
```yaml
name: Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest

    services:
      postgres:
        image: postgres:16
        env:
          POSTGRES_PASSWORD: postgres
        options: >-
          --health-cmd pg_isready
          --health-interval 10s
          --health-timeout 5s
          --health-retries 5

    steps:
      - uses: actions/checkout@v3

      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y libpq-dev valgrind

      - name: Build
        run: make

      - name: Run unit tests
        run: make test

      - name: Run tests with Valgrind
        run: make test VALGRIND=1

      - name: Setup test database
        run: |
          psql -h localhost -U postgres -f tests/setup_test_db.sql

      - name: Run integration tests
        run: make test-integration
```

### Local Test Execution

```bash
# Run all tests
make test

# Run with Valgrind
make test VALGRIND=1

# Run specific test suite
./bin/test-runner --suite lexer

# Run with verbose output
./bin/test-runner --verbose

# Run and generate coverage report
make coverage
```

---

## Test Coverage Goals

### Code Coverage Targets
- **Overall**: > 90%
- **Critical Paths**: 100% (parser, comparison)
- **Error Handling**: > 80%

### Coverage Tools
- `gcov` for line coverage
- `lcov` for HTML reports
- Branch coverage analysis

### Coverage Makefile Target
```makefile
coverage: CFLAGS += -fprofile-arcs -ftest-coverage
coverage: clean test
    lcov --capture --directory . --output-file coverage.info
    genhtml coverage.info --output-directory coverage_html
    @echo "Coverage report: coverage_html/index.html"
```

---

## Success Criteria

### Must Have
- ✅ All unit tests passing
- ✅ All integration tests passing
- ✅ Zero memory leaks (Valgrind clean)
- ✅ > 90% code coverage
- ✅ Parse all Sakila test tables successfully
- ✅ Detect all differences in test fixtures
- ✅ Generate valid PostgreSQL SQL

### Should Have
- ✅ Performance benchmarks met
- ✅ All edge cases covered
- ✅ CI/CD pipeline green
- ✅ Regression test suite

### Nice to Have
- ✅ Fuzzing tests
- ✅ Property-based tests
- ✅ Mutation testing
- ✅ Integration with real-world schemas

---

## Next Steps

### Immediate (Phase 1)
1. Create test framework (`tests/test_framework.h`)
2. Implement test runner (`tests/test_runner.c`)
3. Write memory management tests
4. Write hash table tests
5. Write string builder tests

### Short Term (Phase 2)
6. Write lexer tests
7. Write parser tests
8. Write comparison tests
9. Write output tests

### Medium Term (Phase 3)
10. Create test fixtures
11. Setup test database
12. Write integration tests
13. Write E2E tests

### Long Term (Phase 4-6)
14. Performance testing
15. Regression test suite
16. CI/CD integration
17. Coverage analysis

---

## Test Maintenance

### Adding New Tests
1. Identify test category (unit/integration/e2e)
2. Create test function with descriptive name
3. Add test to appropriate test suite
4. Update test runner
5. Document test purpose and expectations

### Regression Protocol
1. When bug found, create failing test first
2. Fix bug
3. Verify test passes
4. Add test to regression suite
5. Document in CHANGELOG

### Test Review Checklist
- [ ] Test name is descriptive
- [ ] Test is independent (no order dependency)
- [ ] Test cleans up resources
- [ ] Test has clear assertions
- [ ] Test covers both success and failure paths
- [ ] Test is documented

---

## Appendix A: Test Data Sources

### Sakila (DVD Rental) Schema
- 15 tables
- Foreign keys, constraints
- Real-world complexity
- Location: `tests/data/silka/`

### Custom Test Fixtures
- Simple cases: `tests/fixtures/ddl/`
- Edge cases: `tests/fixtures/ddl/edge_cases/`
- Expected outputs: `tests/fixtures/expected/`

### PostgreSQL Documentation Examples
- Official CREATE TABLE examples
- Edge cases from PostgreSQL docs
- Version-specific features

---

## Appendix B: Testing Tools

### Required Tools
- **gcc**: Compiler with C11 support
- **make**: Build automation
- **PostgreSQL**: libpq-dev for compilation
- **Valgrind**: Memory leak detection

### Optional Tools
- **gdb**: Debugging
- **lcov**: Coverage reports
- **clang-tidy**: Static analysis
- **cppcheck**: Additional static analysis
- **AddressSanitizer**: Runtime error detection
- **UndefinedBehaviorSanitizer**: UB detection

### Recommended Setup
```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install gcc make postgresql libpq-dev valgrind \
    lcov gdb clang-tidy cppcheck

# Install dependencies (Arch Linux)
sudo pacman -S gcc make postgresql-libs valgrind lcov gdb \
    clang cppcheck

# Install dependencies (macOS)
brew install gcc make postgresql valgrind lcov
```
