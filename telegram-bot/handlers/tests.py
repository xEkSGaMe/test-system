# telegram-bot/handlers/tests.py (обновить)
from aiogram import Router, types, F
from aiogram.filters import Command
from aiogram.fsm.context import FSMContext
from aiogram.fsm.state import State, StatesGroup
from services.core_service import core_service
from services.redis_client import redis_client

router = Router()


class TestStates(StatesGroup):
    waiting_for_answer = State()


@router.message(Command("tests"))
async def cmd_tests(message: types.Message):
    """Показать список доступных тестов"""
    # 1. Получить access_token из Redis
    session = await redis_client.get_user_session(message.from_user.id)
    if not session or "access_token" not in session:
        await message.answer("❌ Сначала авторизуйтесь через /login")
        return

    # 2. Запросить тесты из Core API
    tests = await core_service.get_tests(session["access_token"])

    if not tests:
        await message.answer("📭 Тестов пока нет")
        return

    # 3. Показать список тестов
    keyboard = types.InlineKeyboardMarkup(inline_keyboard=[])
    for test in tests:
        keyboard.inline_keyboard.append([
            types.InlineKeyboardButton(
                text=f"📝 {test['title']}",
                callback_data=f"start_test:{test['id']}"
            )
        ])

    await message.answer(
        "📚 *Доступные тесты:*\n\n" +
        "\n".join([f"{i + 1}. {t['title']} - {t.get('description', '')}"
                   for i, t in enumerate(tests)]),
        parse_mode="Markdown",
        reply_markup=keyboard
    )


@router.callback_query(F.data.startswith("start_test:"))
async def start_test(callback: types.CallbackQuery, state: FSMContext):
    """Начать прохождение теста"""
    test_id = int(callback.data.split(":")[1])

    # Получить токен
    session = await redis_client.get_user_session(callback.from_user.id)
    if not session:
        await callback.answer("❌ Нет сессии")
        return

    # Запросить тест
    test = await core_service.get_test(test_id, session["access_token"])
    if not test:
        await callback.answer("❌ Тест не найден")
        return

    # Сохранить состояние
    await state.set_state(TestStates.waiting_for_answer)
    await state.update_data(
        test_id=test_id,
        questions=test.get("questions", []),
        current_question=0,
        answers=[],
        test_title=test.get("title", "Тест")
    )

    # Показать первый вопрос
    await show_question(callback.message, state)


async def show_question(message: types.Message, state: FSMContext):
    """Показать текущий вопрос"""
    data = await state.get_data()
    questions = data.get("questions", [])
    current = data.get("current_question", 0)

    if current >= len(questions):
        # Тест завершен
        await finish_test(message, state)
        return

    question = questions[current]
    keyboard = types.InlineKeyboardMarkup(inline_keyboard=[])

    for i, option in enumerate(question.get("options", [])):
        keyboard.inline_keyboard.append([
            types.InlineKeyboardButton(
                text=option,
                callback_data=f"answer:{i}"
            )
        ])

    await message.answer(
        f"❓ *Вопрос {current + 1}/{len(questions)}*\n\n"
        f"{question.get('text', '')}",
        parse_mode="Markdown",
        reply_markup=keyboard
    )