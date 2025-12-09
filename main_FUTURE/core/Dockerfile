FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libpqxx-dev \
    libhiredis-dev \
    libssl-dev \
    wget \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN mkdir -p build && cd build && \
    cmake .. && \
    make

FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libpqxx-6.1 \
    libhiredis1.1 \
    libssl3 \
    wget \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/build/core-service .

EXPOSE 8082

HEALTHCHECK --interval=30s --timeout=10s --start-period=50s --retries=3 \
    CMD wget --no-verbose --tries=1 -O /dev/null http://localhost:8082/health || exit 1

CMD ["./core-service"]