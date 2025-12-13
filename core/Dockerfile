# Stage 1: build
FROM debian:bookworm-slim AS build
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake libpq-dev libpqxx-dev pkg-config \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . /src

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -- -j$(nproc)

# Stage 2: runtime
FROM debian:bookworm-slim

# Устанавливаем runtime зависимости и клиентские утилиты PostgreSQL (pg_isready)
RUN apt-get update && apt-get install -y --no-install-recommends \
    libpq5 libpqxx-6.4 libstdc++6 ca-certificates curl postgresql-client \
  && rm -rf /var/lib/apt/lists/*

# Создаём непривилегированного пользователя
RUN addgroup --system app && adduser --system --ingroup app app

# Копируем бинарь из стадии сборки
COPY --from=build /src/build/core-api /usr/local/bin/core-api
RUN chmod +x /usr/local/bin/core-api && chown app:app /usr/local/bin/core-api

EXPOSE 8082

HEALTHCHECK --interval=10s --timeout=3s --start-period=10s --retries=3 \
  CMD curl -f http://localhost:8082/health || exit 1

USER app
WORKDIR /app
CMD ["/usr/local/bin/core-api"]
