# PostgreSQL Schema Compare Tool

A high-performance C tool for comparing and synchronizing PostgreSQL schemas between DDL files and live databases.

## Quick Start

```bash
# Build the tool
make

# Compare directory schema to database and generate migration
schema-compare --source ./schema/ --target postgresql://user:pass@localhost:5432/mydb --output migration.sql

# Update multiple databases with a single migration file
schema-compare --source ./schema/ --target postgresql://user:pass@prod:5432/db1 --target postgresql://user:pass@prod:5432/db2 --output migration.sql
```

## Features

- **Multi-Target Migrations**: Generate a single migration SQL file that can update multiple target databases
- **Flexible Source Input**: Compare from DDL files in directories or directly from a source database
- **Database-to-Database Comparison**: Synchronize schemas between production, staging, and development environments
- **SQL Migration Generation**: Automatically generates ALTER TABLE, CREATE TABLE, and DROP statements
- **Memory Safe**: Context-based memory management prevents leaks in complex operations
- **Transaction Support**: Wraps migrations in transactions for safe rollback (optional)
- **Schema Filtering**: Target specific schemas within databases

## Installation

### Prerequisites

- GCC or compatible C compiler
- PostgreSQL development libraries:
  - Debian/Ubuntu: `sudo apt-get install libpq-dev`
  - RHEL/CentOS: `sudo yum install postgresql-devel`
  - macOS: `brew install postgresql`
- GNU Make

### Build

```bash
# Clone the repository
git clone <repository-url>
cd schema-compare-c

# Build release version
make release

# Install (optional)
sudo make install
```

## Usage

### Command Syntax

```bash
schema-compare --source SOURCE --target TARGET [--target TARGET2 ...] [OPTIONS]
```

### Source Options

The `--source` can be:
- **Directory path**: A directory containing `.sql` DDL files
  - Example: `--source ./schema/`
  - Example: `--source /path/to/ddl/files/`
- **Database URI**: A PostgreSQL database connection string
  - Example: `--source postgresql://user:password@host:5432/database`

### Target Options

The `--target` must be a PostgreSQL database URI (directories are not supported as targets):
- **Database URI**: `postgresql://user:password@host:port/database`
- **Multiple targets**: Specify `--target` multiple times to generate a single migration for multiple databases

### Common Options

- `--output FILE` or `-o FILE`: Write migration SQL to file (default: stdout)
- `--schema SCHEMA`: Specify schema name (default: `public`)
- `--no-transactions`: Don't wrap SQL in BEGIN/COMMIT transactions
- `--verbose` or `-v`: Enable verbose logging
- `--quiet` or `-q`: Suppress non-error output
- `--help` or `-h`: Show help message
- `--version` or `-V`: Show version information

## Examples

### Compare Directory Schema to Single Database

```bash
schema-compare --source ./schema/ --target postgresql://admin:secret@localhost:5432/production --output migrate.sql
```

Generates `migrate.sql` with ALTER/CREATE/DROP statements to make the `production` database match the DDL files in `./schema/`.

### Compare Directory Schema to Multiple Databases

```bash
schema-compare \
  --source ./schema/ \
  --target postgresql://user:pass@db1.example.com:5432/app \
  --target postgresql://user:pass@db2.example.com:5432/app \
  --output migration.sql
```

Generates a single `migration.sql` file that can be applied to both target databases to bring them in sync with the schema directory.

### Compare One Database to Another

```bash
schema-compare \
  --source postgresql://user:pass@prod.example.com:5432/maindb \
  --target postgresql://user:pass@staging.example.com:5432/maindb \
  --output sync-staging.sql
```

Generates SQL to make the staging database match the production database schema.

### Specify Non-Public Schema

```bash
schema-compare \
  --source ./schema/ \
  --target postgresql://user:pass@localhost:5432/mydb \
  --schema inventory \
  --output migration.sql
```

Compares and generates migration for the `inventory` schema instead of the default `public` schema.

### Generate Migration Without Transactions

```bash
schema-compare \
  --source ./schema/ \
  --target postgresql://user:pass@localhost:5432/mydb \
  --no-transactions \
  --output migration.sql
```

Generates migration SQL without wrapping statements in BEGIN/COMMIT, useful for manual execution or when specific transaction control is needed.

## Connection String Format

PostgreSQL connection URIs follow the standard format:

```
postgresql://[user[:password]@][host[:port]][/database][?parameters]
```

### Components

- **Protocol**: `postgresql://` (required)
- **User**: Database username (optional, defaults to system user)
- **Password**: Database password (optional, prompted if omitted)
- **Host**: Server hostname or IP (default: `localhost`)
- **Port**: PostgreSQL port (default: `5432`)
- **Database**: Database name (required)

### Examples

```bash
# Full connection string
postgresql://admin:secret@db.example.com:5432/production

# Without password (will prompt)
postgresql://admin@db.example.com:5432/production

# Localhost with defaults
postgresql://user:pass@localhost/mydb

# With connection parameters
postgresql://user:pass@host:5432/db?sslmode=require
```

### Security Note

Avoid including passwords in command history. Consider using:
- `.pgpass` file for password storage
- Environment variables: `PGPASSWORD=secret schema-compare ...`
- Connection strings without passwords (tool will prompt)

## Output Format

The tool generates SQL migration scripts containing:

- **CREATE TABLE** statements for new tables
- **ALTER TABLE** statements for schema modifications:
  - ADD COLUMN for new columns
  - DROP COLUMN for removed columns
  - ALTER COLUMN for type/constraint changes
  - ADD CONSTRAINT for new constraints
  - DROP CONSTRAINT for removed constraints
- **DROP TABLE** statements for removed tables (with CASCADE when needed)

### Transaction Wrapping

By default, migrations are wrapped in transactions:

```sql
BEGIN;

-- Migration statements here

COMMIT;
```

Use `--no-transactions` to disable this behavior.

### Dependency Ordering

The tool automatically orders statements to respect dependencies:
1. DROP operations (constraints, then tables)
2. CREATE TABLE operations
3. ALTER TABLE operations
4. ADD CONSTRAINT operations

This ensures migrations can be executed without dependency errors.

## License

This project is licensed under the GNU General Public License v3.0 (GPLv3). See the LICENSE file for details.
