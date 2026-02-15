-- database/migrations/01_init.sql
-- Инициализация базы данных PostgreSQL

-- Таблица пользователей (синхронизируется с MongoDB)
CREATE TABLE IF NOT EXISTS users (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    email VARCHAR(255) UNIQUE NOT NULL,
    full_name VARCHAR(255) NOT NULL,
    roles JSONB DEFAULT '["student"]'::jsonb,
    is_blocked BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

COMMENT ON TABLE users IS 'Пользователи системы (основная информация)';
COMMENT ON COLUMN users.roles IS 'Роли пользователя: student, teacher, admin';

-- Таблица дисциплин (курсов)
CREATE TABLE IF NOT EXISTS courses (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    title VARCHAR(255) NOT NULL,
    description TEXT,
    teacher_id UUID REFERENCES users(id) ON DELETE SET NULL,
    is_deleted BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_courses_teacher_id ON courses(teacher_id);
COMMENT ON TABLE courses IS 'Дисциплины/курсы для тестирования';

-- Таблица записей на курсы
CREATE TABLE IF NOT EXISTS enrollments (
    user_id UUID REFERENCES users(id) ON DELETE CASCADE,
    course_id UUID REFERENCES courses(id) ON DELETE CASCADE,
    enrolled_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (user_id, course_id)
);

CREATE INDEX IF NOT EXISTS idx_enrollments_user_id ON enrollments(user_id);
CREATE INDEX IF NOT EXISTS idx_enrollments_course_id ON enrollments(course_id);
COMMENT ON TABLE enrollments IS 'Связь пользователей с курсами';

-- Таблица вопросов
CREATE TABLE IF NOT EXISTS questions (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    text TEXT NOT NULL,
    options JSONB NOT NULL CHECK (jsonb_typeof(options) = 'array'),
    correct_option INTEGER NOT NULL CHECK (correct_option >= 0),
    points INTEGER DEFAULT 1 CHECK (points > 0),
    author_id UUID REFERENCES users(id) ON DELETE SET NULL,
    version INTEGER DEFAULT 1,
    is_deleted BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_questions_author_id ON questions(author_id);
CREATE INDEX IF NOT EXISTS idx_questions_version ON questions(version);
COMMENT ON TABLE questions IS 'Вопросы для тестов';
COMMENT ON COLUMN questions.options IS 'JSON массив вариантов ответа';
COMMENT ON COLUMN questions.correct_option IS 'Индекс правильного ответа (0-based)';

-- Таблица тестов
CREATE TABLE IF NOT EXISTS tests (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    course_id UUID REFERENCES courses(id) ON DELETE CASCADE,
    title VARCHAR(255) NOT NULL,
    description TEXT,
    time_limit_minutes INTEGER DEFAULT 60 CHECK (time_limit_minutes > 0),
    max_attempts INTEGER DEFAULT 1 CHECK (max_attempts > 0),
    is_active BOOLEAN DEFAULT FALSE,
    is_deleted BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_tests_course_id ON tests(course_id);
CREATE INDEX IF NOT EXISTS idx_tests_is_active ON tests(is_active);
COMMENT ON TABLE tests IS 'Тесты (наборы вопросов)';

-- Таблица связи тестов и вопросов
CREATE TABLE IF NOT EXISTS test_questions (
    test_id UUID REFERENCES tests(id) ON DELETE CASCADE,
    question_id UUID REFERENCES questions(id) ON DELETE CASCADE,
    question_order INTEGER NOT NULL CHECK (question_order >= 0),
    question_snapshot JSONB, -- Снимок вопроса на момент добавления в тест
    PRIMARY KEY (test_id, question_id)
);

CREATE INDEX IF NOT EXISTS idx_test_questions_test_id ON test_questions(test_id);
CREATE INDEX IF NOT EXISTS idx_test_questions_question_id ON test_questions(question_id);
CREATE INDEX IF NOT EXISTS idx_test_questions_order ON test_questions(question_order);
COMMENT ON TABLE test_questions IS 'Связь тестов с вопросами';
COMMENT ON COLUMN test_questions.question_snapshot IS 'Снимок вопроса для сохранения неизменности';

-- Таблица попыток прохождения теста
CREATE TABLE IF NOT EXISTS attempts (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID REFERENCES users(id) ON DELETE CASCADE,
    test_id UUID REFERENCES tests(id) ON DELETE CASCADE,
    started_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    finished_at TIMESTAMP WITH TIME ZONE,
    score INTEGER DEFAULT 0 CHECK (score >= 0),
    max_score INTEGER DEFAULT 0 CHECK (max_score >= 0),
    status VARCHAR(50) DEFAULT 'in_progress' CHECK (status IN ('in_progress', 'finished', 'expired', 'cancelled')),
    answers JSONB DEFAULT '{}'::jsonb,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_attempts_user_id ON attempts(user_id);
CREATE INDEX IF NOT EXISTS idx_attempts_test_id ON attempts(test_id);
CREATE INDEX IF NOT EXISTS idx_attempts_status ON attempts(status);
CREATE INDEX IF NOT EXISTS idx_attempts_created_at ON attempts(created_at);
COMMENT ON TABLE attempts IS 'Попытки прохождения тестов';
COMMENT ON COLUMN attempts.answers IS 'JSON с ответами пользователя {question_id: selected_option}';

-- Таблица ответов (детальная)
CREATE TABLE IF NOT EXISTS answers (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    attempt_id UUID REFERENCES attempts(id) ON DELETE CASCADE,
    question_id UUID REFERENCES questions(id) ON DELETE SET NULL,
    question_snapshot JSONB NOT NULL,
    selected_option INTEGER CHECK (selected_option >= -1), -- -1 означает не отвечено
    is_correct BOOLEAN,
    points_earned INTEGER DEFAULT 0,
    answered_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_answers_attempt_id ON answers(attempt_id);
CREATE INDEX IF NOT EXISTS idx_answers_question_id ON answers(question_id);
COMMENT ON TABLE answers IS 'Детальные ответы на вопросы в рамках попытки';

-- Функция для обновления updated_at
CREATE OR REPLACE FUNCTION update_updated_at_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = CURRENT_TIMESTAMP;
    RETURN NEW;
END;
$$ language 'plpgsql';

-- Триггеры для автоматического обновления updated_at
CREATE TRIGGER update_users_updated_at 
    BEFORE UPDATE ON users 
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_courses_updated_at 
    BEFORE UPDATE ON courses 
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_questions_updated_at 
    BEFORE UPDATE ON questions 
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_tests_updated_at 
    BEFORE UPDATE ON tests 
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_attempts_updated_at 
    BEFORE UPDATE ON attempts 
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

-- Создание пользователей по умолчанию (тестовые данные)
INSERT INTO users (id, email, full_name, roles) VALUES
    ('11111111-1111-1111-1111-111111111111', 'admin@test.com', 'Администратор', '["admin"]'),
    ('22222222-2222-2222-2222-222222222222', 'teacher@test.com', 'Преподаватель', '["teacher"]'),
    ('33333333-3333-3333-3333-333333333333', 'student@test.com', 'Студент', '["student"]')
ON CONFLICT (email) DO NOTHING;