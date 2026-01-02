CREATE TABLE users (
    id INTEGER PRIMARY KEY,
    username TEXT NOT NULL,
    age INTEGER,
    created_at TIMESTAMP DEFAULT now()
);
