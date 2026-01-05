# telegram-bot/bot.py - ИСПРАВЬТЕ НАЧАЛО:
import asyncio
import logging
from aiogram import Bot, Dispatcher
from aiogram.fsm.storage.redis import RedisStorage

from config import Config  # 🔥 ПРЯМОЙ ИМПОРТ

# НАСТРОЙКИ LOGGING
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s"
)
logger = logging.getLogger(__name__)


async def main():
    # 🔥 ПРЯМАЯ ПРОВЕРКА ТОКЕНА
    logger.info(f"Проверка токена: {Config.BOT_TOKEN[:10]}...")

    if not Config.BOT_TOKEN or "7916943118:AAH66weDS-domM749NaVhE_jqdrv65vupq0" not in Config.BOT_TOKEN:
        logger.error("❌ КРИТИЧЕСКАЯ ОШИБКА: Неверный токен бота!")
        logger.error(f"Токен: {Config.BOT_TOKEN}")
        return

    # RedisStorage
    storage = RedisStorage.from_url(
        f"redis://{Config.REDIS_HOST}:{Config.REDIS_PORT}/{Config.REDIS_DB}"
    )

    # Бот и диспетчер
    bot = Bot(token=Config.BOT_TOKEN)
    dp = Dispatcher(storage=storage)

    # Импортируем роутеры
    from handlers.start import router as start_router
    from handlers.auth import router as auth_router
    from handlers.tests import router as tests_router

    dp.include_router(start_router)
    dp.include_router(auth_router)
    dp.include_router(tests_router)

    logger.info("🚀 Запуск бота...")

    try:
        await dp.start_polling(bot)
    except KeyboardInterrupt:
        logger.info("⏹️ Остановлен")
    finally:
        await bot.session.close()


if __name__ == "__main__":
    asyncio.run(main())