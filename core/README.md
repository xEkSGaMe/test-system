🚀 Core API System (C++)

Это центральный модуль системы тестирования. Обеспечивает управление контентом (тесты, вопросы), обработку ответов и автоматический подсчет баллов (Scoring Logic).



🛠 Технологический стек

Язык: C++17



База данных: PostgreSQL (через libpqxx)



JSON: nlohmann/json



Авторизация: JWT (с поддержкой debug-admin-token)



Контейнеризация: Docker (Multi-stage build на Debian Bookworm)



📦 Быстрый запуск

В корне проекта (где лежит docker-compose.yml) выполните:



Bash



docker-compose up --build core-service

Сервер будет доступен по адресу: http://localhost:8080



🔗 Основные Эндпоинты

📝 Тесты и вопросы

GET /tests — получить список всех тестов.



GET /health — проверка состояния сервера и подключения к БД.



🏆 Скоринг (Твоя главная фича)

PUT /attempts/{id}/finish — завершить тест. Сервер автоматически:



Сравнит ответы пользователя с правильными в БД.



Рассчитает процент успеха.



Обновит статус попытки в базе.



Вернет JSON с результатом (score, total, correct).



📊 Мониторинг (Для Никиты)

GET /metrics — метрики в формате Prometheus.



GET /api/metrics — расширенная статистика в формате JSON.



🔐 Безопасность

Для тестирования без модуля Auth используйте заголовок: Authorization: debug-admin-token



📂 Структура модуля

/src/database — логика подключения к PostgreSQL.



/src/services — бизнес-логика (Scoring, CRUD сервисы).



/src/models — структуры данных (Test, Question, Answer).



openapi.yaml — полная спецификация API (Swagger).

