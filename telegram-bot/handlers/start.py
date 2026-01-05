# handlers/start.py - УПРОЩЕННАЯ ВЕРСИЯ
from aiogram import Router, types
from aiogram.filters import Command
from services.redis_client import redis_client

router = Router()


async def cmd_start(message: types.Message):
    """Обработчик /start без deep link"""
    token = await redis_client.get(f"user:{message.from_user.id}:token")

    welcome_text = (
        f"👋 *Привет, {message.from_user.first_name}!*\n\n"
        "🤖 *Бот системы тестирования*\n\n"
    )

    if token:
        welcome_text += (
            "✅ *Вы авторизованы*\n"
            "• /tests - Список тестов\n"
            "• /profile - Ваш профиль\n"
            "• /logout - Выйти\n"
        )
    else:
        welcome_text += (
            "🔐 *Для полного доступа нужна авторизация*\n"
            "• /login - Войти в систему\n"
            "• /help - Помощь\n\n"
            "📝 *Тестовый доступ:*\n"
            "Вы можете использовать /tests для просмотра тестов, "
            "но результаты не будут сохраняться без авторизации."
        )

    from aiogram.types import InlineKeyboardMarkup, InlineKeyboardButton

    keyboard = InlineKeyboardMarkup(inline_keyboard=[
        [InlineKeyboardButton(text="📚 Тесты", callback_data="show_tests")],
        [InlineKeyboardButton(text="🔐 Войти", callback_data="need_auth")],
        [InlineKeyboardButton(text="📋 Помощь", callback_data="show_help")]
    ])

    await message.answer(
        welcome_text,
        parse_mode="Markdown",
        reply_markup=keyboard
    )