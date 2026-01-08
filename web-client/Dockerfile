# Этап 1: Сборка фронтенда (Vite)
FROM node:18-alpine AS build

WORKDIR /app

# Копируем только package.json и lock-файлы сначала (для кэша)
COPY package*.json ./
COPY vite.config.ts ./
COPY tsconfig*.json ./

# Устанавливаем зависимости
RUN npm ci

# Копируем весь исходный код
COPY . .

# Собираем продакшн-версию
RUN npm run build

# Этап 2: Отдача статики через nginx
FROM nginx:alpine

# Копируем собранные файлы из предыдущего этапа
COPY --from=build /app/dist /usr/share/nginx/html

# Копируем кастомный конфиг nginx (для SPA и фикса 404)
COPY nginx.conf /etc/nginx/conf.d/default.conf

EXPOSE 3000