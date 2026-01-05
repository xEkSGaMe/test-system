# handlers/callbacks.py - обработка inline-кнопок

from aiogram import Router, types
from aiogram.filters import Filter
from services.redis_client import redis_client
from keyboards.inline import get_main_menu_keyboard

router = Router()


# Обработка кнопок авторизации
@router.callback_query(lambda c: c.data.startswith("auth_"))
async def process_auth_callback(callback: types.CallbackQuery):
    action = callback.data.split("_")[1]

    if action == "yandex":
        await callback.message.edit_text(
            "🔵 *Авторизация через Яндекс*\n\n"
            "Используйте команду /yauth для получения ссылки.",
            parse_mode="Markdown"
        )
    elif action == "github":
        await callback.message.edit_text(
            "⚫ *Авторизация через GitHub*\n\n"
            "Используйте команду /ghauth для получения ссылки.",
            parse_mode="Markdown"
        )
    elif action == "code":
        await callback.message.edit_text(
            "🔷 *Аутентификация по коду*\n\n"
            "Используйте команду /codeauth для ввода кода.",
            parse_mode="Markdown"
        )

    await callback.answer()


# Проверка авторизации при доступе к тестам
@router.callback_query(lambda c: c.data in ["test_list", "my_results"])
async def check_auth_for_tests(callback: types.CallbackQuery):
    user_id = callback.from_user.id
    token = await redis_client.get(f"user:{user_id}:token")

    if not token:
        await callback.message.edit_text(
            "❌ *Доступ запрещён!*\n\n"
            "Вы не авторизованы. Используйте /start для авторизации.",
            parse_mode="Markdown"
        )
        await callback.answer()
        return

    # Пользователь авторизован - продолжаем
    if callback.data == "test_list":
        await show_test_list(callback)
    elif callback.data == "my_results":
        await show_my_results(callback)