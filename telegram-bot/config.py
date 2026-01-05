import os
from dotenv import load_dotenv

load_dotenv()

class Config:
    # Берем токен из переменной окружения, если её нет - ставим тот, что дал Артём
    BOT_TOKEN = os.getenv("BOT_TOKEN", "7916943118:AAH66weDS-domM749NaVhE_jqdrv65vupq0")

    # В Docker переменные REDIS_HOST и др. придут из docker-compose.yml
    # Если их нет (запуск без Docker), используем localhost
    REDIS_HOST = os.getenv("REDIS_HOST", "localhost")
    REDIS_PORT = int(os.getenv("REDIS_PORT", 6379))
    REDIS_DB = 0
    SESSION_TTL = 86400

    # API URLs - это критично для связи контейнеров
    AUTH_API_URL = os.getenv("AUTH_API_URL", "http://localhost:8081")
    CORE_API_URL = os.getenv("CORE_API_URL", "http://localhost:8080")

    # JWT настройки
    JWT_HEADER = "Authorization"
    JWT_PREFIX = "Bearer"

# Печатаем конфиг при запуске для отладки
print("=" * 50)
print("✅ КОНФИГУРАЦИЯ БОТА ЗАГРУЖЕНА:")
print(f"🤖 Токен: {Config.BOT_TOKEN[:15]}...")
print(f"🔴 Redis Host: {Config.REDIS_HOST}")
print(f"🔵 Auth API: {Config.AUTH_API_URL}")
print(f"🟢 Core API: {Config.CORE_API_URL}")
print("=" * 50)