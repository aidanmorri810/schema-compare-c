-- Test case: Multi-line FOREIGN KEY constraint
-- This causes the parser to hang/infinite loop
-- Similar to what's in customer.sql and film.sql

CREATE TABLE test (
    id INTEGER PRIMARY KEY,
    store_id INTEGER NOT NULL,
    CONSTRAINT test_store_fkey
        FOREIGN KEY (store_id) REFERENCES store(store_id)
);
