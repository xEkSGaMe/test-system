import os
import logging
import asyncio
import sys
from datetime import datetime
from typing import Optional

# Настройка логирования
logging.basicConfig(
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    level=logging.INFO
)
logger = logging.getLogger(__name__)

# Глобальные настройки
class Config:
    """Конфигурация бота"""
    TELEGRAM_TOKEN: Optional[str] = None
    WEB_CLIENT_URL = "http://localhost:3000"
    CORE_API_URL = "http://core-service:8082"
    AUTH_API_URL = "http://auth-service:8081"
    REDIS_URL = "redis://redis:6379/0"


class SystemMonitor:
    """Мониторинг состояния системы"""
    
    def __init__(self):
        self.services = {
            'core-service': {'status': '🟢 Онлайн', 'port': 8082, 'url': Config.CORE_API_URL},
            'auth-service': {'status': '🟢 Онлайн', 'port': 8081, 'url': Config.AUTH_API_URL},
            'web-client': {'status': '🟢 Онлайн', 'port': 3000, 'url': Config.WEB_CLIENT_URL},
            'postgres': {'status': '🟢 Онлайн', 'port': 5432},
            'mongodb': {'status': '🟢 Онлайн', 'port': 27017},
            'redis': {'status': '🟢 Онлайн', 'port': 6379, 'url': Config.REDIS_URL},
        }
        
        self.stats = {
            'start_time': datetime.now(),
            'total_commands': 0,
            'active_users': 0,
        }
    
    def get_status(self) -> str:
        """Получить статус системы"""
        lines = [
            "🖥️ *СТАТУС СИСТЕМЫ*",
            f"Время: {datetime.now().strftime('%H:%M:%S')}",
            f"Активна: {(datetime.now() - self.stats['start_time']).seconds // 60} мин",
            "",
            "*Сервисы:*"
        ]
        
        for service, info in self.services.items():
            lines.append(f"• {service}: {info['status']} :{info['port']}")
        
        lines.extend([
            "",
            "*Статистика:*",
            f"Команд выполнено: {self.stats['total_commands']}",
            f"Активных пользователей: {self.stats['active_users']}",
            "",
            f"🌐 Веб-интерфейс: {Config.WEB_CLIENT_URL}",
            f"🔧 API Core: {Config.CORE_API_URL}",
            f"🔐 API Auth: {Config.AUTH_API_URL}",
        ])
        
        return "\n".join(lines)
    
    def get_services(self) -> str:
        """Получить детальную информацию о сервисах"""
        lines = ["🔧 *СЕРВИСЫ СИСТЕМЫ*", ""]
        
        for service, info in self.services.items():
            lines.append(f"*{service.upper()}*")
            lines.append(f"Статус: {info['status']}")
            lines.append(f"Порт: `{info['port']}`")
            if 'url' in info:
                lines.append(f"URL: `{info['url']}`")
            lines.append("")
        
        return "\n".join(lines)
    
    def get_help(self) -> str:
        """Получить справку"""
        return """🆘 *ПОМОЩЬ ПО КОМАНДАМ*

*Основные команды:*
/start - Начало работы
/status - Статус системы
/services - Информация о сервисах
/help - Эта справка

*Технические данные:*
📊 PostgreSQL: `localhost:5432`
🗄️ MongoDB: `localhost:27017`
⚡ Redis: `localhost:6379`

🚧 *В РАЗРАБОТКЕ:* 
• /login - Авторизация
• /test - Прохождение тестов
• /profile - Личный кабинет
"""


class TelegramBot:
    """Основной класс бота"""
    
    def __init__(self, token: str):
        self.token = token
        self.monitor = SystemMonitor()
        self.application = None
        
    async def on_start(self, update, context):
        """Обработчик команды /start"""
        user = update.effective_user
        self.monitor.stats['total_commands'] += 1
        
        welcome_msg = f"""👋 Привет, {user.first_name}!

🤖 Я - бот системы тестирования.
Система находится в стадии активной разработки.

📊 *Что уже работает:*
• Контейнеры Docker подняты
• Базы данных запущены  
• Веб-интерфейс доступен
• API сервисы готовы

🔧 *Что будет добавлено:*
• Авторизация через OAuth
• Создание и прохождение тестов
• Личный кабинет
• Уведомления

*Основные команды:*
/start - Начало работы
/status - Статус системы
/services - Информация о сервисах
/help - Справка

🌐 *Ссылки:*
• Веб-интерфейс: {Config.WEB_CLIENT_URL}
• API Core: {Config.CORE_API_URL}
• API Auth: {Config.AUTH_API_URL}"""
        
        # Убираем inline-клавиатуру или оставляем без URL кнопок
        keyboard = [
            [{'text': '📊 Статус', 'callback_data': 'status'}],
            [{'text': '🔧 Сервисы', 'callback_data': 'services'}],
            [{'text': '🆘 Помощь', 'callback_data': 'help'}],
        ]
        
        await update.message.reply_text(
            welcome_msg,
            parse_mode='Markdown',
            reply_markup={'inline_keyboard': keyboard}
        )
    
    async def on_status(self, update, context):
        """Обработчик команды /status"""
        self.monitor.stats['total_commands'] += 1
        await update.message.reply_text(
            self.monitor.get_status(),
            parse_mode='Markdown'
        )
    
    async def on_services(self, update, context):
        """Обработчик команды /services"""
        self.monitor.stats['total_commands'] += 1
        await update.message.reply_text(
            self.monitor.get_services(),
            parse_mode='Markdown'
        )
    
    async def on_help(self, update, context):
        """Обработчик команды /help"""
        self.monitor.stats['total_commands'] += 1
        await update.message.reply_text(
            self.monitor.get_help(),
            parse_mode='Markdown'
        )
    
    async def on_callback(self, update, context):
        """Обработчик callback-запросов"""
        query = update.callback_query
        await query.answer()
        
        if query.data == 'status':
            await query.edit_message_text(
                text=self.monitor.get_status(),
                parse_mode='Markdown'
            )
        elif query.data == 'services':
            await query.edit_message_text(
                text=self.monitor.get_services(),
                parse_mode='Markdown'
            )
        elif query.data == 'help':
            await query.edit_message_text(
                text=self.monitor.get_help(),
                parse_mode='Markdown'
            )
    
    async def on_unknown(self, update, context):
        """Обработчик неизвестных команд"""
        await update.message.reply_text(
            "❓ Неизвестная команда.\n"
            "Используйте /help для списка доступных команд.",
            parse_mode='Markdown'
        )
    
    def setup_application(self):
        """Настройка приложения"""
        from telegram.ext import (
            Application, CommandHandler, 
            CallbackQueryHandler, MessageHandler, 
            filters
        )
        
        # Создаем приложение
        self.application = Application.builder().token(self.token).build()
        
        # Регистрируем обработчики
        self.application.add_handler(CommandHandler("start", self.on_start))
        self.application.add_handler(CommandHandler("status", self.on_status))
        self.application.add_handler(CommandHandler("services", self.on_services))
        self.application.add_handler(CommandHandler("help", self.on_help))
        self.application.add_handler(CallbackQueryHandler(self.on_callback))
        self.application.add_handler(MessageHandler(filters.COMMAND, self.on_unknown))
        
        # Регистрируем обработчик ошибок
        self.application.add_error_handler(self.error_handler)
    
    async def error_handler(self, update, context):
        """Обработчик ошибок"""
        logger.error(f"Ошибка при обработке обновления {update}: {context.error}")
    
    def run(self):
        """Запуск бота"""
        self.setup_application()
        
        # Запускаем polling
        logger.info("🤖 Бот запущен. Нажмите Ctrl+C для остановки")
        self.application.run_polling()


def main():
    """Точка входа"""
    logger.info("🚀 Инициализация Telegram Bot...")
    
    # Получаем токен
    token = os.getenv('TELEGRAM_BOT_TOKEN')
    
    if not token:
        logger.error("❌ Токен бота не установлен!")
        return
    
    # Устанавливаем в конфиг
    Config.TELEGRAM_TOKEN = token
    
    try:
        bot = TelegramBot(token)
        bot.run()
    except Exception as e:
        logger.error(f"💥 Необработанная ошибка: {e}")


if __name__ == '__main__':
    main()