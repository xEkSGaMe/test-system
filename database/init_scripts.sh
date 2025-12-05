
### Файл: `database/init_scripts.sh`

```bash
#!/bin/bash

# Скрипт инициализации баз данных системы тестирования
# Автор: System Architect
# Дата: $(date)

set -e  # Выход при ошибке

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Функция для логирования
log_info() {
    echo -e "${BLUE}[INFO]${NC} $(date '+%Y-%m-%d %H:%M:%S') - $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $(date '+%Y-%m-%d %H:%M:%S') - $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $(date '+%Y-%m-%d %H:%M:%S') - $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $(date '+%Y-%m-%d %H:%M:%S') - $1"
}

# Функция проверки доступности сервиса
wait_for_service() {
    local host=$1
    local port=$2
    local service=$3
    local max_attempts=30
    local attempt=1
    
    log_info "Ожидание доступности $service ($host:$port)..."
    
    while ! nc -z $host $port >/dev/null 2>&1; do
        if [ $attempt -eq $max_attempts ]; then
            log_error "$service не доступен после $max_attempts попыток"
            return 1
        fi
        
        log_info "Попытка $attempt/$max_attempts: $service еще не готов..."
        sleep 2
        attempt=$((attempt + 1))
    done
    
    log_success "$service доступен!"
}

# Функция выполнения SQL файла
execute_sql_file() {
    local file=$1
    local db=$2
    local user=$3
    local password=$4
    local host=$5
    local port=$6
    
    log_info "Выполнение SQL файла: $file"
    
    if [ -f "$file" ]; then
        PGPASSWORD="$password" psql -h "$host" -p "$port" -U "$user" -d "$db" -f "$file"
        
        if [ $? -eq 0 ]; then
            log_success "SQL файл выполнен успешно"
        else
            log_error "Ошибка выполнения SQL файла"
            return 1
        fi
    else
        log_error "SQL файл не найден: $file"
        return 1
    fi
}

# Функция выполнения MongoDB скрипта
execute_mongo_script() {
    local file=$1
    local host=$2
    local port=$3
    
    log_info "Выполнение MongoDB скрипта: $file"
    
    if [ -f "$file" ]; then
        mongosh --host "$host" --port "$port" --username admin --password admin123 --authenticationDatabase admin "$file"
        
        if [ $? -eq 0 ]; then
            log_success "MongoDB скрипт выполнен успешно"
        else
            log_error "Ошибка выполнения MongoDB скрипта"
            return 1
        fi
    else
        log_error "MongoDB файл не найден: $file"
        return 1
    fi
}

# Функция проверки Redis
check_redis() {
    local host=$1
    local port=$2
    local password=$3
    
    log_info "Проверка подключения к Redis..."
    
    if redis-cli -h "$host" -p "$port" -a "$password" ping | grep -q "PONG"; then
        log_success "Redis подключен и отвечает"
        return 0
    else
        log_error "Redis не отвечает"
        return 1
    fi
}

# Основная функция
main() {
    log_info "=== Начало инициализации баз данных системы тестирования ==="
    
    # Проверка наличия необходимых утилит
    command -v psql >/dev/null 2>&1 || { log_error "psql не найден. Установите PostgreSQL client"; exit 1; }
    command -v mongosh >/dev/null 2>&1 || { log_error "mongosh не найден. Установите MongoDB shell"; exit 1; }
    command -v redis-cli >/dev/null 2>&1 || { log_error "redis-cli не найден. Установите Redis client"; exit 1; }
    command -v nc >/dev/null 2>&1 || { log_error "nc (netcat) не найден"; exit 1; }
    
    # Параметры подключения (можно переопределить через переменные окружения)
    POSTGRES_HOST=${POSTGRES_HOST:-localhost}
    POSTGRES_PORT=${POSTGRES_PORT:-5432}
    POSTGRES_DB=${POSTGRES_DB:-test_system}
    POSTGRES_USER=${POSTGRES_USER:-admin}
    POSTGRES_PASSWORD=${POSTGRES_PASSWORD:-admin123}
    
    MONGO_HOST=${MONGO_HOST:-localhost}
    MONGO_PORT=${MONGO_PORT:-27017}
    MONGO_DB=${MONGO_DB:-test_system}
    MONGO_USER=${MONGO_USER:-admin}
    MONGO_PASSWORD=${MONGO_PASSWORD:-admin123}
    
    REDIS_HOST=${REDIS_HOST:-localhost}
    REDIS_PORT=${REDIS_PORT:-6379}
    REDIS_PASSWORD=${REDIS_PASSWORD:-redis123}
    
    # Пути к файлам (относительно текущей директории)
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    POSTGRES_SCHEMA="$SCRIPT_DIR/postgres_schema.sql"
    MONGO_INIT="$SCRIPT_DIR/mongo-init.js"
    
    # Шаг 1: Ожидание доступности сервисов
    wait_for_service "$POSTGRES_HOST" "$POSTGRES_PORT" "PostgreSQL"
    wait_for_service "$MONGO_HOST" "$MONGO_PORT" "MongoDB"
    wait_for_service "$REDIS_HOST" "$REDIS_PORT" "Redis"
    
    # Шаг 2: Проверка Redis
    check_redis "$REDIS_HOST" "$REDIS_PORT" "$REDIS_PASSWORD"
    
    # Шаг 3: Инициализация PostgreSQL
    log_info "Инициализация PostgreSQL базы данных..."
    
    # Проверка существования базы данных
    if PGPASSWORD="$POSTGRES_PASSWORD" psql -h "$POSTGRES_HOST" -p "$POSTGRES_PORT" -U "$POSTGRES_USER" -lqt | cut -d \| -f 1 | grep -qw "$POSTGRES_DB"; then
        log_warning "База данных '$POSTGRES_DB' уже существует"
        read -p "Пересоздать базу данных? (y/N): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            log_info "Удаление существующей базы данных..."
            PGPASSWORD="$POSTGRES_PASSWORD" psql -h "$POSTGRES_HOST" -p "$POSTGRES_PORT" -U "$POSTGRES_USER" -c "DROP DATABASE IF EXISTS $POSTGRES_DB;"
            PGPASSWORD="$POSTGRES_PASSWORD" psql -h "$POSTGRES_HOST" -p "$POSTGRES_PORT" -U "$POSTGRES_USER" -c "CREATE DATABASE $POSTGRES_DB;"
        fi
    else
        log_info "Создание базы данных '$POSTGRES_DB'..."
        PGPASSWORD="$POSTGRES_PASSWORD" psql -h "$POSTGRES_HOST" -p "$POSTGRES_PORT" -U "$POSTGRES_USER" -c "CREATE DATABASE $POSTGRES_DB;"
    fi
    
    # Выполнение SQL схемы
    execute_sql_file "$POSTGRES_SCHEMA" "$POSTGRES_DB" "$POSTGRES_USER" "$POSTGRES_PASSWORD" "$POSTGRES_HOST" "$POSTGRES_PORT"
    
    # Шаг 4: Инициализация MongoDB
    log_info "Инициализация MongoDB..."
    
    # Выполнение MongoDB скрипта
    if [ -f "$MONGO_INIT" ]; then
        execute_mongo_script "$MONGO_INIT" "$MONGO_HOST" "$MONGO_PORT"
    else
        log_warning "MongoDB init скрипт не найден, создание стандартной структуры..."
        
        # Создаем базу данных и коллекцию через mongosh
        mongosh --host "$MONGO_HOST" --port "$MONGO_PORT" --username "$MONGO_USER" --password "$MONGO_PASSWORD" --authenticationDatabase admin <<EOF
use $MONGO_DB

// Создаем коллекцию users
db.createCollection('users')

// Создаем индексы
db.users.createIndex({ "email": 1 }, { unique: true, name: "email_unique" })
db.users.createIndex({ "roles": 1 }, { name: "roles_index" })

// Добавляем тестовых пользователей
const users = [
  {
    email: "student@example.com",
    fullName: "Иван Студентов",
    roles: ["student"],
    isBlocked: false,
    createdAt: new Date(),
    updatedAt: new Date(),
    lastLogin: new Date()
  },
  {
    email: "teacher@example.com",
    fullName: "Петр Преподавателев",
    roles: ["teacher"],
    isBlocked: false,
    createdAt: new Date(),
    updatedAt: new Date(),
    lastLogin: new Date()
  },
  {
    email: "admin@example.com",
    fullName: "Админ Админов",
    roles: ["admin", "teacher"],
    isBlocked: false,
    createdAt: new Date(),
    updatedAt: new Date(),
    lastLogin: new Date()
  }
]

users.forEach(user => {
  db.users.updateOne(
    { email: user.email },
    { \$setOnInsert: user },
    { upsert: true }
  )
})

print("✅ MongoDB инициализирована успешно!")
EOF
    fi
    
    # Шаг 5: Заполнение тестовыми данными
    log_info "Добавление тестовых данных..."
    
    # Добавляем тестовые данные в PostgreSQL
    PGPASSWORD="$POSTGRES_PASSWORD" psql -h "$POSTGRES_HOST" -p "$POSTGRES_PORT" -U "$POSTGRES_USER" -d "$POSTGRES_DB" <<EOF
-- Добавляем дисциплины
INSERT INTO courses (id, title, description, teacher_id) VALUES
    ('aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa', 'Математика', 'Курс высшей математики', '22222222-2222-2222-2222-222222222222'),
    ('bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb', 'Программирование', 'Основы программирования на Python', '22222222-2222-2222-2222-222222222222')
ON CONFLICT DO NOTHING;

-- Записываем студента на курсы
INSERT INTO enrollments (user_id, course_id) VALUES
    ('11111111-1111-1111-1111-111111111111', 'aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa'),
    ('11111111-1111-1111-1111-111111111111', 'bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb')
ON CONFLICT DO NOTHING;

-- Создаем вопросы
INSERT INTO questions (id, text, options, correct_option, author_id) VALUES
    ('cccccccc-cccc-cccc-cccc-cccccccccccc', 'Сколько будет 2+2?', '["1", "2", "3", "4"]', 3, '22222222-2222-2222-2222-222222222222'),
    ('dddddddd-dddd-dddd-dddd-dddddddddddd', 'Столица России?', '["Москва", "Санкт-Петербург", "Казань", "Новосибирск"]', 0, '22222222-2222-2222-2222-222222222222'),
    ('eeeeeeee-eeee-eeee-eeee-eeeeeeeeeeee', 'Python это...', '["Компилируемый язык", "Интерпретируемый язык", "Язык разметки", "База данных"]', 1, '22222222-2222-2222-2222-222222222222')
ON CONFLICT DO NOTHING;

-- Создаем тесты
INSERT INTO tests (id, course_id, title, is_active) VALUES
    ('ffffffff-ffff-ffff-ffff-ffffffffffff', 'aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa', 'Тест по математике', true),
    ('gggggggg-gggg-gggg-gggg-gggggggggggg', 'bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb', 'Тест по программированию', true)
ON CONFLICT DO NOTHING;

-- Связываем тесты и вопросы
INSERT INTO test_questions (test_id, question_id, question_order) VALUES
    ('ffffffff-ffff-ffff-ffff-ffffffffffff', 'cccccccc-cccc-cccc-cccc-cccccccccccc', 0),
    ('ffffffff-ffff-ffff-ffff-ffffffffffff', 'dddddddd-dddd-dddd-dddd-dddddddddddd', 1),
    ('gggggggg-gggg-gggg-gggg-gggggggggggg', 'eeeeeeee-eeee-eeee-eeee-eeeeeeeeeeee', 0)
ON CONFLICT DO NOTHING;

-- Добавляем уведомления
INSERT INTO notifications (id, user_id, title, message, type) VALUES
    ('hhhhhhhh-hhhh-hhhh-hhhh-hhhhhhhhhhhh', '11111111-1111-1111-1111-111111111111', 'Добро пожаловать!', 'Вы успешно зарегистрировались в системе тестирования', 'info'),
    ('iiiiiiii-iiii-iiii-iiii-iiiiiiiiiiii', '22222222-2222-2222-2222-222222222222', 'Новый студент', 'Иван Студентов записался на ваш курс', 'info')
ON CONFLICT DO NOTHING;

EOF
    
    log_success "Тестовые данные добавлены успешно!"
    
    # Шаг 6: Проверка данных
    log_info "Проверка созданных данных..."
    
    echo -e "\n${BLUE}=== Сводка по PostgreSQL ===${NC}"
    PGPASSWORD="$POSTGRES_PASSWORD" psql -h "$POSTGRES_HOST" -p "$POSTGRES_PORT" -U "$POSTGRES_USER" -d "$POSTGRES_DB" -c "
    SELECT 'Пользователи:' as category, COUNT(*) as count FROM users
    UNION ALL
    SELECT 'Курсы:', COUNT(*) FROM courses
    UNION ALL
    SELECT 'Вопросы:', COUNT(*) FROM questions
    UNION ALL
    SELECT 'Тесты:', COUNT(*) FROM tests
    UNION ALL
    SELECT 'Записи на курсы:', COUNT(*) FROM enrollments;
    "
    
    echo -e "\n${BLUE}=== Сводка по MongoDB ===${NC}"
    mongosh --host "$MONGO_HOST" --port "$MONGO_PORT" --username "$MONGO_USER" --password "$MONGO_PASSWORD" --authenticationDatabase admin --quiet <<EOF
use $MONGO_DB
print("Пользователи в MongoDB: " + db.users.countDocuments())
EOF
    
    echo -e "\n${BLUE}=== Проверка Redis ===${NC}"
    redis-cli -h "$REDIS_HOST" -p "$REDIS_PORT" -a "$REDIS_PASSWORD" info memory | grep -E "(used_memory|total_system_memory)"
    
    log_success "=== Инициализация баз данных завершена успешно! ==="
    log_info "Данные для подключения:"
    echo "PostgreSQL: postgres://$POSTGRES_USER:$POSTGRES_PASSWORD@$POSTGRES_HOST:$POSTGRES_PORT/$POSTGRES_DB"
    echo "MongoDB: mongodb://$MONGO_USER:$MONGO_PASSWORD@$MONGO_HOST:$MONGO_PORT/$MONGO_DB"
    echo "Redis: redis://:$REDIS_PASSWORD@$REDIS_HOST:$REDIS_PORT/0"
}

# Запуск основной функции
main "$@"