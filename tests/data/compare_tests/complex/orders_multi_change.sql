CREATE TABLE orders (
    order_id INTEGER PRIMARY KEY,
    customer_id INTEGER NOT NULL,
    discount NUMERIC(5,2) DEFAULT 0,
    total NUMERIC(10,2),
    CONSTRAINT orders_customer_fk
        FOREIGN KEY (customer_id)
        REFERENCES customers(customer_id)
        ON DELETE RESTRICT,
    CONSTRAINT check_discount CHECK (discount >= 0 AND discount <= 1)
);
