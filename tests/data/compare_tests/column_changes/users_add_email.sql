CREATE TABLE users (
    id INTEGER PRIMARY KEY,
    username VARCHAR(50) NOT NULL,
    email VARCHAR(100),
    age INTEGER,
    created_at TIMESTAMP DEFAULT now()
);
