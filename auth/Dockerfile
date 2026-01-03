# auth/Dockerfile
# ИЗМЕНЯЕМ: golang:1.21-alpine → golang:1.23-alpine
FROM golang:1.23-alpine AS builder

WORKDIR /app

# Устанавливаем зависимости для сборки и psql для миграций
RUN apk add --no-cache gcc musl-dev postgresql-client bash

# Копируем go.mod и go.sum
COPY go.mod go.sum ./
RUN go mod download

# Копируем исходный код
COPY . .

# Собираем приложение
RUN CGO_ENABLED=1 GOOS=linux go build -o auth-service ./cmd

# Финальный образ
# ИЗМЕНЯЕМ: alpine:3.20 → alpine:3.18 (для совместимости)
FROM alpine:3.18

# Устанавливаем psql для выполнения миграций
RUN apk --no-cache add ca-certificates postgresql-client bash

WORKDIR /root/

COPY --from=builder /app/auth-service .
COPY --from=builder /app/wait-and-migrate.sh .
COPY --from=builder /app/migrations ./migrations

# Добавляем принудительную конвертацию строк на всякий случай
RUN apk add --no-cache dos2unix && \
    dos2unix wait-and-migrate.sh && \
    chmod +x wait-and-migrate.sh

EXPOSE 8081

# Используй полный путь, если ./ не срабатывает
CMD ["./wait-and-migrate.sh", "postgres", "5432", "admin", "admin123", "test_system", "./migrations", "./auth-service"]