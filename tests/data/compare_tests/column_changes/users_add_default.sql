CREATE TABLE users (
    id INTEGER PRIMARY KEY,
    username VARCHAR(50) NOT NULL,
    age INTEGER DEFAULT 18,
    created_at TIMESTAMP DEFAULT now()
);
