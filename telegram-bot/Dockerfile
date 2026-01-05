FROM python:3.11-alpine

WORKDIR /app

# Устанавливаем системные зависимости для сборки (нужны для библиотек типа yarl или multidict)
RUN apk add --no-cache gcc musl-dev linux-headers postgresql-dev

# Сначала копируем и устанавливаем зависимости (для кэширования слоев)
COPY requirements.txt .
RUN pip install --no-cache-dir --upgrade pip && \
    pip install --no-cache-dir -r requirements.txt

# Копируем ВЕСЬ проект бота (папки handlers, services, database и т.д.)
COPY . .

# Удаляем лишнее, если оно просочилось
RUN rm -f package.json test_results.db

# Настройка прав доступа
RUN adduser -D -u 1000 botuser && \
    chown -R botuser:botuser /app
USER botuser

# Запускаем бота
CMD ["python", "-u", "bot.py"]