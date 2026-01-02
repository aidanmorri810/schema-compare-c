# PostgreSQL Schema Compare Tool

A modular C tool for comparing PostgreSQL schemas between DDL files and live databases.

## Overview

This tool compares PostgreSQL table definitions from:
- DDL files (`.sql` files containing CREATE TABLE statements)
- Live PostgreSQL databases

It generates:
- Human-readable difference reports
- SQL migration scripts (ALTER TABLE statements) to synchronize schemas

## Features

- **Modular Architecture**: Clean separation between parsing, database introspection, comparison, and output generation
- **Memory Management**: Context-based allocation system prevents memory leaks
- **Extensible**: Easy to add support for new DDL types (indexes, views, functions)
- **Multiple Output Formats**: Text reports and SQL migration scripts

## Build Status

**Phase 1 Complete**: Foundation modules implemented
- ✓ Memory management system
- ✓ Utility modules (strings, errors, hash tables, file I/O)
- ✓ Build system (Makefile)

## Building

### Prerequisites

- GCC or compatible C compiler
- PostgreSQL development libraries (`libpq-dev` on Debian/Ubuntu)
- GNU Make

### Compilation

```bash
# Build library (development)
make lib

# Build full application (when main.c is implemented)
make

# Debug build
make debug

# Release build
make release

# Clean build artifacts
make clean
```

## Project Structure

```
schema-compare-c/
├── include/           # Header files
│   ├── pg_create_table.h  # PostgreSQL DDL structures
│   ├── memory.h           # Memory management API
│   └── utils.h            # Utility functions
├── src/
│   ├── memory/        # Memory context implementation
│   ├── utils/         # String utils, errors, hash tables, file I/O
│   ├── parser/        # DDL parser (to be implemented)
│   ├── db_reader/     # Database introspection (to be implemented)
│   ├── compare/       # Schema comparison engine (to be implemented)
│   └── output/        # Report and SQL generation (to be implemented)
├── tests/             # Unit tests (to be implemented)
├── Makefile           # Build system
└── README.md          # This file
```

## Architecture

### Module Design

```
Parser Module ──→ CreateTableStmt structures ←── DB Reader Module
                         ↓
                  Memory Manager
                         ↓
                  Comparison Engine
                         ↓
          ┌──────────────┴──────────────┐
    Report Generator           SQL Generator
```

### Key Components

#### 1. Memory Management (`memory.h/c`)
- Context-based allocation tracking
- Automatic cleanup when context is destroyed
- Prevents memory leaks in complex data structures

#### 2. Utilities (`utils.h`)
- **String utilities**: trim, concat, split, case conversion
- **String builder**: Efficient string concatenation
- **Error handling**: Structured error reporting
- **Logging**: Debug, info, warn, error levels
- **Hash tables**: O(1) lookups for schema comparison
- **File I/O**: Read files, scan directories

#### 3. DDL Structures (`pg_create_table.h`)
Complete representation of PostgreSQL CREATE TABLE syntax:
- Regular tables, typed tables, partition tables
- All constraint types (NOT NULL, CHECK, UNIQUE, PK, FK, EXCLUDE)
- Partitioning (RANGE, LIST, HASH)
- Storage parameters, tablespaces, inheritance

## Development Roadmap

### Phase 1: Foundation ✓
- [x] Memory management
- [x] Utility modules
- [x] Build system

### Phase 2: Parser (In Progress)
- [ ] Lexer/tokenizer
- [ ] Recursive descent parser
- [ ] CREATE TABLE parsing
- [ ] Constraint parsing
- [ ] Partition specification parsing

### Phase 3: Database Reader
- [ ] libpq connection management
- [ ] System catalog queries
- [ ] Build CreateTableStmt from database

### Phase 4: Comparison Engine
- [ ] Schema-level comparison
- [ ] Table-level comparison
- [ ] Column and constraint comparison
- [ ] Difference detection and categorization

### Phase 5: Output Generation
- [ ] Human-readable text reports
- [ ] SQL migration script generation
- [ ] Dependency ordering

### Phase 6: CLI Application
- [ ] Command-line argument parsing
- [ ] Workflow orchestration
- [ ] Main entry point

### Phase 7: Testing
- [ ] Unit tests for each module
- [ ] Integration tests
- [ ] Test fixtures

## Usage (Planned)

```bash
# Compare database schema to DDL files
schema-compare --source db://localhost/mydb --target ./ddl-files/ --report text

# Generate SQL migration script
schema-compare --source ./ddl-v1/ --target ./ddl-v2/ --output migration.sql

# Compare two databases
schema-compare --source db://prod/appdb --target db://dev/appdb --schema public
```

## Extending the Tool

The modular design makes it easy to extend:

### Adding New DDL Types (e.g., CREATE INDEX)
1. Create structure definitions (e.g., `pg_create_index.h`)
2. Add parser functions (e.g., `parse_index.c`)
3. Add database introspection (e.g., `db_index.c`)
4. Add comparison logic (e.g., `compare_index.c`)
5. Add SQL generation (e.g., `sql_gen_index.c`)

### Adding Output Formats (e.g., JSON)
1. Implement format generator (e.g., `generate_json_report()`)
2. Register in report module
3. Add CLI option

## License

[To be determined]

## Contributing

[To be determined]
