CREATE TABLE users (
    id INTEGER PRIMARY KEY,
    username VARCHAR(50) NOT NULL UNIQUE,
    age INTEGER,
    created_at TIMESTAMP DEFAULT now()
);
