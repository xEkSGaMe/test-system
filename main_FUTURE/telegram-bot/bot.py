import os
import logging
import redis
from telegram import Update
from telegram.ext import Application, CommandHandler, ContextTypes

logging.basicConfig(
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    level=logging.INFO
)
logger = logging.getLogger(__name__)

redis_url = os.getenv('REDIS_URL', 'redis://:redis123@redis:6379/2')
redis_client = redis.from_url(redis_url)

async def start(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    user = update.effective_user
    await update.message.reply_text(
        f"Привет, {user.first_name}!\n\n"
        f"Test System Telegram Bot (Python)\n"
        f"Версия: Under Construction\n\n"
        f"Команды:\n"
        f"/start - Начать работу\n"
        f"/help - Помощь\n"
        f"/health - Проверить статус"
    )

async def help_command(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    await update.message.reply_text(
        "Доступные команды:\n"
        "/start - Начать работу\n"
        "/help - Эта справка\n"
        "/health - Проверить здоровье системы"
    )

async def health_check(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    try:
        redis_status = "✅" if redis_client.ping() else "❌"
        
        await update.message.reply_text(
            f"Проверка здоровья системы:\n"
            f"Redis: {redis_status}\n"
            f"Bot: ✅ Работает\n"
            f"\nВсе системы в норме!"
        )
    except Exception as e:
        await update.message.reply_text(f"❌ Ошибка проверки здоровья: {str(e)}")

def main() -> None:
    token = os.getenv('TELEGRAM_BOT_TOKEN')
    if not token:
        logger.error("TELEGRAM_BOT_TOKEN не установлен!")
        return
    
    application = Application.builder().token(token).build()
    
    application.add_handler(CommandHandler("start", start))
    application.add_handler(CommandHandler("help", help_command))
    application.add_handler(CommandHandler("health", health_check))
    
    logger.info("Бот запущен...")
    application.run_polling(allowed_updates=Update.ALL_TYPES)

if __name__ == '__main__':
    main()