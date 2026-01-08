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
    print(f"DEBUG: State set for user {callback.from_user.id}: {await state.get_state()}")

    # Показать первый вопрос
    await show_question(callback.message, state)


async def show_question(message: types.Message, state: FSMContext):
    """Показать текущий вопрос"""
    print(f"DEBUG: Showing question for user {message.chat.id}. State: {await state.get_state()}")
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


@router.callback_query(TestStates.waiting_for_answer, F.data.startswith("answer:"))
async def handle_answer(callback: types.CallbackQuery, state: FSMContext):
    print(f"DEBUG: Callback received: {callback.data}. State: {await state.get_state()}. User: {callback.from_user.id}")
    try:
        await callback.answer("Ответ принят!")  # Покажет уведомление пользователю (уберёт "часики" и подтвердит)
        data = await state.get_data()
        current = data.get("current_question", 0)
        answers = data.get("answers", [])
        
        # 2. Получаем индекс ответа (например, из 'answer:0' вытащит 0)
        answer_idx = int(callback.data.split(":")[1])
        answers.append(answer_idx)
        
        # 3. Обновляем счетчик вопросов
        await state.update_data(current_question=current + 1, answers=answers)
        
        # 4. Скрываем кнопки текущего вопроса, чтобы не нажимали повторно
        await callback.message.edit_reply_markup(reply_markup=None)
        
        # 5. Вызываем показ следующего вопроса
        await show_question(callback.message, state)
    except Exception as e:
        print(f"ERROR in handle_answer: {str(e)}")  # Логируем ошибку в консоль
        await callback.answer("Ошибка! Попробуйте заново.")  # Показываем пользователю


async def finish_test(message: types.Message, state: FSMContext):
    """Завершение теста и отправка результатов"""
    data = await state.get_data()
    test_id = data.get("test_id")
    answers = data.get("answers", [])
    test_title = data.get("test_title", "Тест")

    # Получаем токен для отправки результатов в API
    session = await redis_client.get_user_session(message.chat.id)
    
    # Здесь можно отправить результаты в Core API (POST /attempts)
    # Но пока просто поздравим пользователя
    
    await message.answer(
        f"🏁 *Тест «{test_title}» завершен!*\n\n"
        f"Вы ответили на {len(answers)} вопр. Спасибо за участие!",
        parse_mode="Markdown"
    )
    
    # Сбрасываем состояние
    await state.clear()