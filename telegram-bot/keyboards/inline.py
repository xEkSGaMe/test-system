# keyboards/inline.py - ДОБАВИТЬ новые клавиатуры

from aiogram.types import InlineKeyboardMarkup, InlineKeyboardButton
from aiogram.utils.keyboard import InlineKeyboardBuilder


def get_auth_methods_keyboard() -> InlineKeyboardMarkup:
    """Клавиатура выбора способа авторизации"""
    builder = InlineKeyboardBuilder()

    builder.row(
        InlineKeyboardButton(
            text="🔵 Яндекс",
            callback_data="auth_yandex"
        ),
        InlineKeyboardButton(
            text="⚫ GitHub",
            callback_data="auth_github"
        )
    )

    builder.row(
        InlineKeyboardButton(
            text="🔷 По коду",
            callback_data="auth_code"
        )
    )

    builder.row(
        InlineKeyboardButton(
            text="📋 Помощь",
            callback_data="auth_help"
        )
    )

    return builder.as_markup()


def get_main_menu_keyboard(is_authenticated: bool = False) -> InlineKeyboardMarkup:
    """Главное меню после авторизации"""
    builder = InlineKeyboardBuilder()

    if is_authenticated:
        builder.row(
            InlineKeyboardButton(
                text="📝 Пройти тест",
                callback_data="test_list"
            )
        )
        builder.row(
            InlineKeyboardButton(
                text="📊 Мои результаты",
                callback_data="my_results"
            ),
            InlineKeyboardButton(
                text="🏆 Топ результатов",
                callback_data="leaderboard"
            )
        )
        builder.row(
            InlineKeyboardButton(
                text="⚙️ Настройки",
                callback_data="settings"
            ),
            InlineKeyboardButton(
                text="🚪 Выйти",
                callback_data="logout"
            )
        )
    else:
        builder.row(
            InlineKeyboardButton(
                text="🔐 Авторизоваться",
                callback_data="need_auth"
            )
        )

    return builder.as_markup()