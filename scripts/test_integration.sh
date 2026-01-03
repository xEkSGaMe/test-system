#!/bin/bash

echo "=== Testing Core API and Auth Service Integration ==="

# Тест 1: Проверка health endpoints
echo -e "\n1. Testing health endpoints:"
curl -s http://localhost:8080/health | jq .
curl -s http://localhost:8081/health | jq .

# Тест 2: Получение debug token от auth-service
echo -e "\n2. Getting debug token from auth-service:"
TOKEN=$(curl -s -X POST http://localhost:8081/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin"}' | jq -r '.token')
echo "Token: ${TOKEN:0:50}..."

# Тест 3: Создание теста с токеном
echo -e "\n3. Creating test with token:"
curl -X POST http://localhost:8080/tests \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $TOKEN" \
  -d '{"title":"Integration Test","description":"Test created via API with auth"}'

# Тест 4: Получение списка тестов
echo -e "\n4. Getting tests list:"
curl -s http://localhost:8080/tests | jq .

echo -e "\n=== Integration test completed ==="