-- Создание таблицы refresh_tokens для auth service
CREATE TABLE IF NOT EXISTS refresh_tokens (
    id SERIAL PRIMARY KEY,
    user_id INT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    token_hash VARCHAR(255) NOT NULL UNIQUE,
    expires_at TIMESTAMP NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

-- Индексы для refresh_tokens
CREATE INDEX IF NOT EXISTS idx_refresh_tokens_user_id ON refresh_tokens(user_id);
CREATE INDEX IF NOT EXISTS idx_refresh_tokens_expires_at ON refresh_tokens(expires_at);
CREATE INDEX IF NOT EXISTS idx_refresh_tokens_token_hash ON refresh_tokens(token_hash);

-- Добавляем поле для JWT blacklist если используется PostgreSQL вместо Redis
CREATE TABLE IF NOT EXISTS blacklisted_tokens (
    id SERIAL PRIMARY KEY,
    token_hash VARCHAR(512) NOT NULL UNIQUE,
    expires_at TIMESTAMP NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

-- Индексы для blacklisted_tokens
CREATE INDEX IF NOT EXISTS idx_blacklisted_tokens_token_hash ON blacklisted_tokens(token_hash);
CREATE INDEX IF NOT EXISTS idx_blacklisted_tokens_expires_at ON blacklisted_tokens(expires_at);

-- Обновляем пароль администратора на правильный bcrypt хеш
-- Пароль: admin123, хеш: $2a$10$N9qo8uLOickgx2ZMRZoMye7Z7lW4c6q1b5f7g8B1JdTkO9V1vY5zG
UPDATE users 
SET password_hash = '$2a$10$N9qo8uLOickgx2ZMRZoMye7Z7lW4c6q1b5f7g8B1JdTkO9V1vY5zG' 
WHERE email = 'admin@example.com' AND password_hash = '$2a$10$replace_with_bcrypt_hash';

-- Добавляем тестового пользователя для проверки
INSERT INTO users (email, password_hash, full_name, role, is_blocked, created_at, updated_at)
SELECT 'test@example.com', '$2a$10$N9qo8uLOickgx2ZMRZoMye7Z7lW4c6q1b5f7g8B1JdTkO9V1vY5zG', 'Test User', 'student', FALSE, NOW(), NOW()
WHERE NOT EXISTS (SELECT 1 FROM users WHERE email = 'test@example.com');