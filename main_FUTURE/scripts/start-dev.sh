#!/bin/bash
# Запуск для разработки

echo "🚀 Запуск тестовой системы в режиме разработки..."

# Создаем сеть если не существует
docker network create test-network 2>/dev/null || true

# Запускаем базы данных
echo "1. Запуск баз данных..."
docker-compose up -d postgres mongodb redis

# Ждем запуска баз
echo "   Ожидание запуска баз данных..."
sleep 10

# Запускаем сервисы с hot reload
echo "2. Запуск сервисов разработки..."
docker-compose -f docker-compose.dev.yml up -d

echo "✅ Система запущена!"
echo ""
echo "🌐 Доступные сервисы:"
echo "   Web Client:     http://localhost:3000"
echo "   Auth Service:   http://localhost:8081"
echo "   Core Service:   http://localhost:8082"
echo "   Telegram Bot:   http://localhost:8083"
echo ""
echo "📊 Статус:"
docker-compose ps