-- Initial database setup (runs on container startup)
-- This file is executed by docker-entrypoint-initdb.d

-- Enable extensions that might be needed for testing
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

-- Create a dedicated test schema (in addition to public)
CREATE SCHEMA IF NOT EXISTS test_schema;

-- Grant permissions
GRANT ALL PRIVILEGES ON DATABASE schema_compare_test TO testuser;
GRANT ALL PRIVILEGES ON SCHEMA public TO testuser;
GRANT ALL PRIVILEGES ON SCHEMA test_schema TO testuser;

-- Set search path
ALTER DATABASE schema_compare_test SET search_path TO public, test_schema;
