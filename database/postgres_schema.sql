-- Полная схема базы данных системы тестирования
-- PostgreSQL 15

-- Расширение для UUID
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

-- Таблица пользователей (из PostgreSQL)
CREATE TABLE users (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    email VARCHAR(255) UNIQUE NOT NULL,
    full_name VARCHAR(255) NOT NULL,
    roles JSONB DEFAULT '["student"]'::jsonb,
    is_blocked BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- Таблица дисциплин (курсов)
CREATE TABLE courses (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    title VARCHAR(255) NOT NULL,
    description TEXT,
    teacher_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    is_deleted BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- Таблица вопросов
CREATE TABLE questions (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    text TEXT NOT NULL,
    options JSONB NOT NULL, -- Массив вариантов ответов
    correct_option INTEGER NOT NULL, -- Индекс правильного ответа (0-based)
    author_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    version INTEGER DEFAULT 1,
    is_deleted BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- Таблица тестов
CREATE TABLE tests (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    course_id UUID NOT NULL REFERENCES courses(id) ON DELETE CASCADE,
    title VARCHAR(255) NOT NULL,
    is_active BOOLEAN DEFAULT FALSE,
    is_deleted BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- Таблица связи тестов и вопросов (многие-ко-многим)
CREATE TABLE test_questions (
    test_id UUID NOT NULL REFERENCES tests(id) ON DELETE CASCADE,
    question_id UUID NOT NULL REFERENCES questions(id) ON DELETE CASCADE,
    question_order INTEGER NOT NULL, -- Порядок вопросов в тесте
    PRIMARY KEY (test_id, question_id)
);

-- Таблица попыток прохождения теста
CREATE TABLE attempts (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    test_id UUID NOT NULL REFERENCES tests(id) ON DELETE CASCADE,
    question_snapshots JSONB NOT NULL, -- Снимки вопросов на момент начала теста
    is_finished BOOLEAN DEFAULT FALSE,
    started_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    finished_at TIMESTAMP WITH TIME ZONE,
    score DECIMAL(5,2) DEFAULT 0.0
);

-- Таблица ответов в попытках
CREATE TABLE answers (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    question_id UUID NOT NULL REFERENCES questions(id) ON DELETE CASCADE,
    attempt_id UUID NOT NULL REFERENCES attempts(id) ON DELETE CASCADE,
    selected_option INTEGER DEFAULT -1, -- -1 означает "нет ответа"
    is_correct BOOLEAN,
    answered_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- Таблица записи на курсы
CREATE TABLE enrollments (
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    course_id UUID NOT NULL REFERENCES courses(id) ON DELETE CASCADE,
    enrolled_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (user_id, course_id)
);

-- Таблица уведомлений
CREATE TABLE notifications (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    title VARCHAR(255) NOT NULL,
    message TEXT NOT NULL,
    type VARCHAR(50) DEFAULT 'info',
    is_read BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    read_at TIMESTAMP WITH TIME ZONE
);

-- Таблица логов действий пользователей
CREATE TABLE user_activity_logs (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id UUID REFERENCES users(id) ON DELETE SET NULL,
    action VARCHAR(100) NOT NULL,
    entity_type VARCHAR(50),
    entity_id UUID,
    details JSONB,
    ip_address INET,
    user_agent TEXT,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- Индексы для ускорения поиска

-- Для пользователей
CREATE INDEX idx_users_email ON users(email);
CREATE INDEX idx_users_blocked ON users(is_blocked) WHERE is_blocked = true;

-- Для курсов
CREATE INDEX idx_courses_teacher ON courses(teacher_id);
CREATE INDEX idx_courses_deleted ON courses(is_deleted) WHERE is_deleted = true;

-- Для вопросов
CREATE INDEX idx_questions_author ON questions(author_id);
CREATE INDEX idx_questions_deleted ON questions(is_deleted) WHERE is_deleted = true;

-- Для тестов
CREATE INDEX idx_tests_course ON tests(course_id);
CREATE INDEX idx_tests_active ON tests(is_active) WHERE is_active = true;
CREATE INDEX idx_tests_deleted ON tests(is_deleted) WHERE is_deleted = true;

-- Для попыток
CREATE INDEX idx_attempts_user_test ON attempts(user_id, test_id);
CREATE INDEX idx_attempts_finished ON attempts(is_finished) WHERE is_finished = true;
CREATE INDEX idx_attempts_started ON attempts(started_at);

-- Для ответов
CREATE INDEX idx_answers_attempt ON answers(attempt_id);
CREATE INDEX idx_answers_question ON answers(question_id);

-- Для записей на курсы
CREATE INDEX idx_enrollments_user ON enrollments(user_id);
CREATE INDEX idx_enrollments_course ON enrollments(course_id);

-- Для уведомлений
CREATE INDEX idx_notifications_user ON notifications(user_id);
CREATE INDEX idx_notifications_read ON notifications(is_read) WHERE is_read = false;
CREATE INDEX idx_notifications_created ON notifications(created_at);

-- Для логов
CREATE INDEX idx_logs_user ON user_activity_logs(user_id);
CREATE INDEX idx_logs_action ON user_activity_logs(action);
CREATE INDEX idx_logs_created ON user_activity_logs(created_at);

-- Триггеры для автоматического обновления updated_at
CREATE OR REPLACE FUNCTION update_updated_at_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = CURRENT_TIMESTAMP;
    RETURN NEW;
END;
$$ language 'plpgsql';

-- Применяем триггеры к таблицам
CREATE TRIGGER update_users_updated_at BEFORE UPDATE ON users
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_courses_updated_at BEFORE UPDATE ON courses
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_questions_updated_at BEFORE UPDATE ON questions
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_tests_updated_at BEFORE UPDATE ON tests
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

-- Функция для расчета оценки теста
CREATE OR REPLACE FUNCTION calculate_attempt_score(attempt_uuid UUID)
RETURNS DECIMAL(5,2) AS $$
DECLARE
    total_questions INTEGER;
    correct_answers INTEGER;
BEGIN
    SELECT COUNT(*)
    INTO total_questions
    FROM answers a
    WHERE a.attempt_id = attempt_uuid;
    
    SELECT COUNT(*)
    INTO correct_answers
    FROM answers a
    JOIN questions q ON a.question_id = q.id
    WHERE a.attempt_id = attempt_uuid 
      AND a.selected_option = q.correct_option;
    
    IF total_questions = 0 THEN
        RETURN 0;
    END IF;
    
    RETURN (correct_answers::DECIMAL / total_questions::DECIMAL) * 100;
END;
$$ LANGUAGE plpgsql;

-- Комментарии к таблицам
COMMENT ON TABLE users IS 'Таблица пользователей системы';
COMMENT ON TABLE courses IS 'Таблица учебных дисциплин (курсов)';
COMMENT ON TABLE questions IS 'Таблица вопросов для тестов';
COMMENT ON TABLE tests IS 'Таблица тестов';
COMMENT ON TABLE test_questions IS 'Таблица связи тестов и вопросов';
COMMENT ON TABLE attempts IS 'Таблица попыток прохождения тестов';
COMMENT ON TABLE answers IS 'Таблица ответов пользователей в попытках';
COMMENT ON TABLE enrollments IS 'Таблица записи пользователей на курсы';
COMMENT ON TABLE notifications IS 'Таблица уведомлений пользователей';
COMMENT ON TABLE user_activity_logs IS 'Таблица логов действий пользователей';

-- Начальные данные для тестирования
INSERT INTO users (id, email, full_name, roles) VALUES
    ('11111111-1111-1111-1111-111111111111', 'student@example.com', 'Иван Студентов', '["student"]'),
    ('22222222-2222-2222-2222-222222222222', 'teacher@example.com', 'Петр Преподавателев', '["teacher"]'),
    ('33333333-3333-3333-3333-333333333333', 'admin@example.com', 'Админ Админов', '["admin", "teacher"]')
ON CONFLICT (email) DO NOTHING;