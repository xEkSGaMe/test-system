FROM alpine:latest

# Устанавливаем компилятор и библиотеки
RUN apk add --no-cache g++ musl-dev

WORKDIR /app

# Копируем исходник
COPY src/main.cpp .

# Компилируем
RUN g++ -std=c++11 -static -o core-api main.cpp

EXPOSE 8080

CMD ["./core-api"]
