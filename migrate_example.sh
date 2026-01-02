#!/bin/bash

# Example: Migrate schema from directory to Docker database

DIR="tests/data/nora/src/nora-tenant"
DB_CONN="host=localhost port=5433 dbname=schema_compare_test user=testuser password=testpass"
OUTPUT_SQL="migration.sql"

echo "Generating migration from $DIR to database..."
./bin/schema-compare --sql "$OUTPUT_SQL" --no-transactions "$DB_CONN" "$DIR"

echo ""
echo "To apply the migration, run:"
echo "  PGPASSWORD=testpass psql -h localhost -p 5433 -U testuser -d schema_compare_test -f $OUTPUT_SQL"
