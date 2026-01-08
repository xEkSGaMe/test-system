# handlers/auth.py
from aiogram import Router, types, F
from aiogram.filters import Command
from aiogram.types import InlineKeyboardButton, InlineKeyboardMarkup
from aiogram.fsm.context import FSMContext
from aiogram.fsm.state import State, StatesGroup
from services.api_client import APIClient
from services.redis_client import redis_client
import logging

router = Router()
logger = logging.getLogger(__name__)
api_client = APIClient()


class AuthStates(StatesGroup):
    waiting_for_oauth = State()


@router.message(Command("login"))
async def cmd_login(message: types.Message):
    """Команда для входа через OAuth"""
    # Исправлено: получаем сессию, а не просто токен
    session = await redis_client.get_user_session(message.from_user.id)
    token = session.get("access_token") if session else None

    if token:
        validation = await api_client.validate_token(token)
        if validation.get("valid"):
            await message.answer(
                "✅ Вы уже авторизованы!\n"
                "Используйте /tests для просмотра доступных тестов."
            )
            return

    keyboard = InlineKeyboardMarkup(inline_keyboard=[
        [
            InlineKeyboardButton(text="🔵 Яндекс", callback_data="auth_yandex"),
            InlineKeyboardButton(text="⚫ GitHub", callback_data="auth_github")
        ]
    ])

    await message.answer(
        "🔐 *Авторизация через OAuth*\n\nВыберите способ входа:",
        reply_markup=keyboard,
        parse_mode="Markdown"
    )


# В handlers/auth.py ЗАМЕНИ эти функции:

@router.callback_query(F.data == "auth_yandex")
async def auth_yandex(callback: types.CallbackQuery, state: FSMContext):
    """Авторизация через Яндекс"""
    # 🔴 ИСПРАВЬ: не передавай user_id в функцию
    auth_url = await api_client.get_yandex_auth_url()

    if auth_url:
        await state.set_state(AuthStates.waiting_for_oauth)
        await state.update_data(auth_method="yandex")

        await callback.message.answer(
            "🔵 *Авторизация через Яндекс*\n\n"
            "1. Нажмите на ссылку ниже\n"
            "2. Разрешите доступ приложению\n"
            "3. Вы будете перенаправлены обратно в бота\n\n"
            f"Ссылка: `{auth_url}`\n\n"
            "⚠️ *После авторизации вернитесь в бота*",
            parse_mode="Markdown"
        )
        await callback.answer()
    else:
        await callback.answer(
            "❌ Не удалось получить ссылку для авторизации. Проверьте настройки Auth сервиса.",
            show_alert=True
        )


@router.callback_query(F.data == "auth_github")
async def auth_github(callback: types.CallbackQuery, state: FSMContext):
    """Авторизация через GitHub"""
    # 🔴 ИСПРАВЬ: не передавай user_id в функцию
    auth_url = await api_client.get_github_auth_url()

    if auth_url:
        await state.set_state(AuthStates.waiting_for_oauth)
        await state.update_data(auth_method="github")

        await callback.message.answer(
            "⚫ *Авторизация через GitHub*\n\n"
            "1. Нажмите на ссылку ниже\n"
            "2. Разрешите доступ приложению\n"
            "3. Вы будете перенаправлены обратно в бота\n\n"
            f"Ссылка: `{auth_url}`\n\n"
            "⚠️ *После авторизации вернитесь в бота*",
            parse_mode="Markdown"
        )
        await callback.answer()
    else:
        await callback.answer(
            "❌ Не удалось получить ссылку для авторизации. Проверьте настройки Auth сервиса.",
            show_alert=True
        )


@router.message(Command("start"))
async def handle_deep_link(message: types.Message):
    """Обработка deep link (t.me/bot?start=tk...)"""
    args = message.text.split()
    
    # 1. Если просто /start без параметров
    if len(args) <= 1:
        await message.answer(
            "🤖 *Добро пожаловать!*\n\n"
            "Используйте /login для авторизации\n"
            "/tests для списка тестов\n"
            "/profile для вашего профиля",
            parse_mode="Markdown"
        )
        return

    payload = args[1]
    token = None

    # 2. Если пришел ТИКЕТ (начинается на tk)
    if payload.startswith("tk"):
        msg = await message.answer("⏳ *Авторизация...* Пожалуйста, подождите.", parse_mode="Markdown")
        # ОБМЕНИВАЕМ ТИКЕТ НА ТОКЕН
        token = await api_client.exchange_ticket(payload)
        await msg.delete()
    else:
        # Если вдруг пришел сразу JWT (старая логика)
        token = payload

    if not token:
        await message.answer(
            "❌ *Ошибка авторизации*\n\n"
            "Ссылка устарела или недействительна. Попробуйте войти снова через /login",
            parse_mode="Markdown"
        )
        return

    # 3. Валидируем полученный токен
    validation = await api_client.validate_token(token)

    if validation.get("valid"):
        session_data = {"access_token": token}
        await redis_client.set_user_session(message.from_user.id, session_data)

        # Пытаемся получить данные профиля
        profile = await api_client.get_user_profile(token)
        
        # В твоем Go-сервисе данные лежат в ключе "data" (судя по Me обработчику)
        user_info = profile.get("data", {}) if profile.get("success") else {}

        await message.answer(
            f"✅ *Авторизация успешна!*\n\n"
            f"👤 *Пользователь:* {user_info.get('email', 'Пользователь')}\n"
            f"🎓 *Роль:* {user_info.get('role', 'student')}\n\n"
            f"Теперь вам доступны все функции бота!",
            parse_mode="Markdown"
        )
    else:
        await message.answer("❌ Сервер отклонил токен. Попробуйте /login еще раз.")

@router.message(Command("profile"))
async def cmd_profile(message: types.Message):
    """Показать профиль пользователя"""
    session = await redis_client.get_user_session(message.from_user.id)
    token = session.get("access_token") if session else None

    if not token:
        await message.answer("❌ Вы не авторизованы.")
        return

    validation = await api_client.validate_token(token)
    if not validation.get("valid"):
        # Исправлено: имя метода в redis_client.py -> delete_user_session
        await redis_client.delete_user_session(message.from_user.id)
        await message.answer("❌ *Сессия истекла*. Войдите снова через /login")
        return

    profile = await api_client.get_user_profile(token)
    if profile.get("success") and "data" in profile:
        user_info = profile["data"]
        await message.answer(
            f"👤 *Ваш профиль*\n\n"
            f"📧 *Email:* {user_info.get('email')}\n"
            f"🎓 *Роль:* {user_info.get('role')}",
            parse_mode="Markdown"
        )
    else:
        await message.answer("❌ Ошибка получения профиля")


@router.message(Command("logout"))
async def cmd_logout(message: types.Message):
    """Выйти из системы"""
    # Исправлено: имя метода в redis_client.py -> delete_user_session
    success = await redis_client.delete_user_session(message.from_user.id)
    
    if success:
        await message.answer("✅ *Вы успешно вышли из системы*", parse_mode="Markdown")
    else:
        await message.answer("ℹ️ Вы и так не авторизованы.")