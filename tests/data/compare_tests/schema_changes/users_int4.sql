CREATE TABLE users (
    id int4 PRIMARY KEY,
    username VARCHAR(50) NOT NULL,
    age int4,
    created_at TIMESTAMP DEFAULT now()
);
