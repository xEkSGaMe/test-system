#!/bin/bash
# scripts/init-databases.sh
# Скрипт инициализации всех баз данных

set -e

echo "🚀 Начало инициализации баз данных..."
echo "======================================"

# Проверяем переменные окружения
check_env() {
    if [ -z "$POSTGRES_USER" ] || [ -z "$POSTGRES_PASSWORD" ]; then
        echo "❌ Ошибка: Переменные окружения PostgreSQL не установлены"
        exit 1
    fi
}

# Ждем доступности сервисов
wait_for_service() {
    local host=$1
    local port=$2
    local name=$3
    local max_attempts=30
    local attempt=1
    
    echo "⏳ Ожидание доступности $name ($host:$port)..."
    
    while ! nc -z "$host" "$port" >/dev/null 2>&1; do
        if [ $attempt -eq $max_attempts ]; then
            echo "❌ $name не доступен после $max_attempts попыток"
            exit 1
        fi
        
        echo "Попытка $attempt/$max_attempts..."
        sleep 2
        attempt=$((attempt + 1))
    done
    
    echo "✅ $name доступен"
}

# Инициализация PostgreSQL
init_postgres() {
    echo "📊 Инициализация PostgreSQL..."
    
    # Проверяем, нужно ли выполнить миграции
    local migrations_dir="./database/migrations"
    
    if [ -d "$migrations_dir" ]; then
        for migration in $(ls $migrations_dir/*.sql | sort); do
            echo "Применение миграции: $(basename $migration)"
            PGPASSWORD="$POSTGRES_PASSWORD" psql -h postgres -U "$POSTGRES_USER" -d test_system -f "$migration"
        done
    else
        echo "ℹ️ Директория миграций не найдена"
    fi
    
    echo "✅ PostgreSQL инициализирован"
}

# Инициализация MongoDB
init_mongodb() {
    echo "📄 Инициализация MongoDB..."
    
    # Проверяем, есть ли файл инициализации
    local init_file="./database/mongo-init.js"
    
    if [ -f "$init_file" ]; then
        echo "Выполнение скрипта инициализации MongoDB..."
        mongosh --host mongodb --port 27017 -u admin -p admin123 --authenticationDatabase admin "$init_file"
    else
        echo "ℹ️ Файл инициализации MongoDB не найден"
    fi
    
    echo "✅ MongoDB инициализирован"
}

# Проверка Redis
check_redis() {
    echo "🔍 Проверка Redis..."
    
    if redis-cli -h redis -a redis123 ping | grep -q "PONG"; then
        echo "✅ Redis доступен"
    else
        echo "❌ Redis не отвечает"
        exit 1
    fi
}

# Создание тестовых данных
create_test_data() {
    echo "🎲 Создание тестовых данных..."
    
    # Создаем тестовые данные в PostgreSQL
    PGPASSWORD="$POSTGRES_PASSWORD" psql -h postgres -U "$POSTGRES_USER" -d test_system -c "
        -- Создаем тестовые курсы
        INSERT INTO courses (id, title, description, teacher_id) VALUES
            ('44444444-4444-4444-4444-444444444444', 'Математика', 'Базовый курс математики', '22222222-2222-2222-2222-222222222222'),
            ('55555555-5555-5555-5555-555555555555', 'Программирование', 'Введение в программирование', '22222222-2222-2222-2222-222222222222')
        ON CONFLICT DO NOTHING;
        
        -- Создаем тестовые вопросы
        INSERT INTO questions (id, text, options, correct_option, points, author_id) VALUES
            ('66666666-6666-6666-6666-666666666666', 'Сколько будет 2+2?', '[\"3\", \"4\", \"5\", \"6\"]', 1, 2, '22222222-2222-2222-2222-222222222222'),
            ('77777777-7777-7777-7777-777777777777', 'Что такое переменная?', '[\"Число\", \"Строка\", \"Контейнер для данных\", \"Функция\"]', 2, 3, '22222222-2222-2222-2222-222222222222')
        ON CONFLICT DO NOTHING;
    "
    
    echo "✅ Тестовые данные созданы"
}

# Основная функция
main() {
    echo "🕐 Время начала: $(date)"
    
    # Проверяем окружение
    check_env
    
    # Ждем доступности сервисов
    wait_for_service "postgres" 5432 "PostgreSQL"
    wait_for_service "mongodb" 27017 "MongoDB"
    wait_for_service "redis" 6379 "Redis"
    
    # Инициализируем базы данных
    init_postgres
    init_mongodb
    check_redis
    
    # Создаем тестовые данные
    create_test_data
    
    echo "======================================"
    echo "🎉 Инициализация завершена успешно!"
    echo "🕐 Время окончания: $(date)"
}

# Запускаем основную функцию
main