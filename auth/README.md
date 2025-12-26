Auth Service (Go)

📌 Описание

Auth‑сервис отвечает за аутентификацию и авторизацию пользователей в системе тестирования.

Он реализован на Go, использует PostgreSQL и Redis для хранения данных и поддерживает JWT‑токены с ролями.

Все запросы к Core API проходят через проверку прав, выданных этим модулем.



🚀 Возможности

Регистрация новых пользователей (authregister)



Логин и выдача пары токенов (access\_token, refresh\_token)



Валидация токена (authvalidate)



Обновление токена (authrefresh)



Выход и инвалидирование токена (authlogout)



Получение информации о текущем пользователе (authme)



Хранение refresh‑токенов и blacklist в PostgreSQL



Поддержка Redis для масштабируемого хранения сессий



JWT‑токены содержат user\_id, email, role



⚙️ Технологии

Go 1.23+



Gin (HTTP‑фреймворк)



PostgreSQL (хранение пользователей и токенов)



Redis (сессии, кэш)



jwt‑go  jwt‑cpp (работа с токенами)



Docker Compose (оркестрация сервисов)



📂 Структура

Код

auth

├── cmd                # точка входа main.go

├── internal

│   ├── handlers       # HTTP‑эндпоинты

│   ├── services       # бизнес‑логика

│   ├── repositories   # работа с БД

│   ├── models         # структуры данных

│   └── utils          # вспомогательные функции

├── migrations         # SQL‑миграции

├── Dockerfile

└── docker-compose.yml

🔑 Переменные окружения

POSTGRES\_HOST — хост PostgreSQL



POSTGRES\_USER — пользователь БД



POSTGRES\_PASSWORD — пароль БД



POSTGRES\_DB — имя базы



REDIS\_HOST — хост Redis



JWT\_SECRET — секрет для подписи JWT



GIN\_MODE — режим работы (debug или release)



📡 Эндпоинты

Метод	Путь	Описание

GET	health	Проверка статуса сервиса

POST	authregister	Регистрация нового пользователя

POST	authlogin	Логин, выдача токенов

POST	authvalidate	Проверка валидности access‑токена

POST	authrefresh	Обновление токенов

POST	authlogout	Выход, инвалидирование токена

GET	authme	Информация о текущем пользователе

🧪 Примеры запросов

Регистрация

bash

curl -X POST httplocalhost8081authregister 

&nbsp; -H Content-Type applicationjson 

&nbsp; -d '{emailuser@example.com,passwordPassw0rd!,full\_nameUser One}'

Логин

bash

curl -X POST httplocalhost8081authlogin 

&nbsp; -H Content-Type applicationjson 

&nbsp; -d '{emailuser@example.com,passwordPassw0rd!}'

Refresh

bash

curl -X POST httplocalhost8081authrefresh 

&nbsp; -H Content-Type applicationjson 

&nbsp; -d '{refresh\_tokenrefresh\_token}'

📈 Статус выполнения

✅ Регистрация, логин, JWT



✅ Refresh‑токены и logout



✅ PostgreSQL + Redis интеграция



⚠️ Документация SwaggerOpenAPI (в процессе)



❌ OAuth2 (GoogleGitHub) — планируется

