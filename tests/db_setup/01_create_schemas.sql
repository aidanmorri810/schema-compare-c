-- Create additional test schemas for multi-schema testing
CREATE SCHEMA IF NOT EXISTS schema_a;
CREATE SCHEMA IF NOT EXISTS schema_b;

GRANT ALL PRIVILEGES ON SCHEMA schema_a TO testuser;
GRANT ALL PRIVILEGES ON SCHEMA schema_b TO testuser;
