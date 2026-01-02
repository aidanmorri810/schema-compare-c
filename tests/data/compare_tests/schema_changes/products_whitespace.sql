CREATE TABLE products (
    product_id INTEGER PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    price NUMERIC(8,2) NOT NULL,
    stock INTEGER DEFAULT  0
);
