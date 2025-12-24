FROM python:3.11-alpine

WORKDIR /app

# Устанавливаем системные зависимости для сборки
RUN apk add --no-cache gcc musl-dev linux-headers postgresql-dev

# Копируем зависимости
COPY requirements.txt .
RUN pip install --no-cache-dir --upgrade pip && \
    pip install --no-cache-dir -r requirements.txt

# Копируем код бота
COPY bot.py .
COPY package.json .

# Создаем не-root пользователя
RUN adduser -D -u 1000 botuser && \
    chown -R botuser:botuser /app
USER botuser

# Запускаем бота
CMD ["python", "-u", "bot.py"]