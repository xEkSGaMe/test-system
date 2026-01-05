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
    user_id = message.from_user.id  #
    # Проверяем, уже авторизован ли пользователь
    token = await redis_client.get_access_token(message.from_user.id)

    if token:
        # Проверяем валидность токена
        validation = await api_client.validate_token(token)
        if validation.get("valid"):
            await message.answer(
                "✅ Вы уже авторизованы!\n"
                "Используйте /tests для просмотра доступных тестов.\n"
                "Или /profile для просмотра профиля."
            )
            return
        else:
            # Токен невалиден, удаляем
            await redis_client.get_access_token(message.from_user.id)

    # Предлагаем способы авторизации
    keyboard = InlineKeyboardMarkup(inline_keyboard=[
        [
            InlineKeyboardButton(text="🔵 Яндекс", callback_data="auth_yandex"),
            InlineKeyboardButton(text="⚫ GitHub", callback_data="auth_github")
        ]
    ])

    await message.answer(
        "🔐 *Авторизация через OAuth*\n\n"
        "Для доступа ко всем функциям бота необходимо авторизоваться.\n"
        "Выберите способ входа:\n\n"
        "⚠️ *Внимание:* После авторизации вы будете перенаправлены обратно в бота с токеном.",
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
    """Обработка deep link с токеном (t.me/bot?start=token)"""
    # Проверяем, есть ли токен в команде /start
    if len(message.text.split()) > 1:
        token = message.text.split()[1]  # /start <token>

        # Проверяем валидность токена
        validation = await api_client.validate_token(token)

        if validation.get("valid"):
            # Сохраняем токен в Redis
            await redis_client.set(
                f"user:{message.from_user.id}:token",
                token,
                ex=3600  # 1 час (обычное время жизни access token)
            )

            # Получаем профиль пользователя
            profile = await api_client.get_user_profile(token)

            if "user" in profile:
                user_info = profile["user"]
                await message.answer(
                    f"✅ *Авторизация успешна!*\n\n"
                    f"👤 *Пользователь:* {user_info.get('full_name', 'Неизвестно')}\n"
                    f"📧 *Email:* {user_info.get('email', 'Не указан')}\n"
                    f"🎓 *Роль:* {user_info.get('role', 'student')}\n\n"
                    f"Теперь вам доступны все функции бота!",
                    parse_mode="Markdown"
                )
            else:
                await message.answer(
                    "✅ *Авторизация успешна!*\n\n"
                    "Токен сохранен. Теперь вам доступны все функции бота!",
                    parse_mode="Markdown"
                )
        else:
            await message.answer(
                "❌ *Недействительный токен*\n\n"
                "Токен устарел или недействителен.\n"
                "Попробуйте авторизоваться снова через /login",
                parse_mode="Markdown"
            )
    else:
        # Обычный /start без токена
        await message.answer(
            "🤖 *Добро пожаловать!*\n\n"
            "Используйте /login для авторизации\n"
            "/tests для списка тестов\n"
            "/profile для вашего профиля",
            parse_mode="Markdown"
        )


@router.message(Command("profile"))
async def cmd_profile(message: types.Message):
    """Показать профиль пользователя"""
    user_id = message.from_user.id  #
    token = await redis_client.get_access_token(message.from_user.id)

    if not token:
        await message.answer(
            "❌ *Вы не авторизованы*\n\n"
            "Используйте /login для входа в систему.",
            parse_mode="Markdown"
        )
        return

    # Проверяем валидность токена
    validation = await api_client.validate_token(token)
    if not validation.get("valid"):
        await message.answer(
            "❌ *Токен устарел*\n\n"
            "Используйте /login для повторной авторизации.",
            parse_mode="Markdown"
        )
        await redis_client.get_access_token(message.from_user.id)
        return

    # Получаем профиль
    profile = await api_client.get_user_profile(token)

    if "user" in profile:
        user_info = profile["user"]
        await message.answer(
            f"👤 *Ваш профиль*\n\n"
            f"🆔 *ID:* {user_info.get('id', 'Неизвестно')}\n"
            f"📛 *Имя:* {user_info.get('full_name', 'Не указано')}\n"
            f"📧 *Email:* {user_info.get('email', 'Не указан')}\n"
            f"🎓 *Роль:* {user_info.get('role', 'student')}\n\n"
            f"📊 *Статус:* ✅ Авторизован",
            parse_mode="Markdown"
        )
    else:
        await message.answer(
            "❌ *Ошибка получения профиля*\n\n"
            "Не удалось получить информацию о пользователе.",
            parse_mode="Markdown"
        )


@router.message(Command("logout"))
async def cmd_logout(message: types.Message):
    """Выйти из системы"""
    user_id = message.from_user.id  #
    token = await redis_client.get_access_token(message.from_user.id)

    if token:
        await redis_client.get_access_token(message.from_user.id)
        await message.answer(
            "✅ *Вы успешно вышли из системы*\n\n"
            "Ваш токен удален. Для доступа к функциям бота снова используйте /login",
            parse_mode="Markdown"
        )
    else:
        await message.answer(
            "ℹ️ *Вы не авторизованы*\n\n"
            "Используйте /login для входа в систему.",
            parse_mode="Markdown"
        )