# Stage 1: Build
FROM debian:bookworm-slim AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake libpq-dev libpqxx-dev pkg-config \
    ca-certificates wget git libssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Устанавливаем nlohmann/json
RUN mkdir -p /usr/local/include/nlohmann \
    && wget -O /usr/local/include/nlohmann/json.hpp \
    https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp

# Устанавливаем jwt-cpp правильно через репозиторий
RUN git clone https://github.com/Thalhammer/jwt-cpp.git /tmp/jwt-cpp \
    && cd /tmp/jwt-cpp \
    && cmake -B build -DJWT_BUILD_EXAMPLES=OFF \
    && cmake --build build --target install \
    && rm -rf /tmp/jwt-cpp

WORKDIR /src
COPY CMakeLists.txt .
COPY src/ ./src/

# Сборка вашего приложения
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -- -j$(nproc)

# === ЭТАП 2: Запуск (Runtime) ===
FROM debian:bookworm-slim

# Устанавливаем библиотеки, необходимые для работы (libssl3 для JWT и libpqxx для БД)
RUN apt-get update && apt-get install -y --no-install-recommends \
    libpq5 \
    libpqxx-6.4 \
    libssl3 \
    ca-certificates \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Создаем пользователя для безопасности
RUN addgroup --system app && adduser --system --ingroup app app

# Копируем бинарный файл
COPY --from=build /src/build/core-api /usr/local/bin/core-api
RUN chmod +x /usr/local/bin/core-api && chown app:app /usr/local/bin/core-api

USER app
WORKDIR /app

EXPOSE 8080

CMD ["/usr/local/bin/core-api"]