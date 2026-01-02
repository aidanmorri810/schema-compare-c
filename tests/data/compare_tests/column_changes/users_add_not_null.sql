CREATE TABLE users (
    id INTEGER PRIMARY KEY,
    username VARCHAR(50) NOT NULL,
    age INTEGER NOT NULL,
    created_at TIMESTAMP DEFAULT now()
);
