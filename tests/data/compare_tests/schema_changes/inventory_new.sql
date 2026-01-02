CREATE TABLE inventory (
    inventory_id INTEGER PRIMARY KEY,
    product_id INTEGER NOT NULL,
    quantity INTEGER DEFAULT 0,
    warehouse_location VARCHAR(100)
);
