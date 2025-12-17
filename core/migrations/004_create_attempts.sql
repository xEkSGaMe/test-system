CREATE TABLE IF NOT EXISTS attempts (
    id SERIAL PRIMARY KEY,
    user_id INT NOT NULL,
    test_id INT NOT NULL REFERENCES tests(id) ON DELETE CASCADE,
    answers JSONB,
    status VARCHAR(32) NOT NULL DEFAULT 'in_progress',
    started_at TIMESTAMP NOT NULL DEFAULT NOW(),
    finished_at TIMESTAMP
);
