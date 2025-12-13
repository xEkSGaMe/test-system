FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libpq-dev \
    libpqxx-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app

RUN cmake -S . -B build && cmake --build build --config Release \
    && cp build/core-api /usr/local/bin/core-api

EXPOSE 8080
CMD ["core-api"]
