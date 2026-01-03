-- Добавляем user_id в таблицу tests для связи с пользователями из Auth
ALTER TABLE tests ADD COLUMN user_id INTEGER;

-- Обновляем существующие записи (ставим user_id = 1 для admin)
UPDATE tests SET user_id = 1 WHERE user_id IS NULL;

-- Делаем поле обязательным
ALTER TABLE tests ALTER COLUMN user_id SET NOT NULL;

-- Создаем индекс для быстрого поиска тестов пользователя
CREATE INDEX idx_tests_user_id ON tests(user_id);

-- Комментарий для документации
COMMENT ON COLUMN tests.user_id IS 'ID пользователя-создателя теста (из Auth сервиса)';