# test_token.py
import asyncio
from aiogram import Bot

TOKEN = "7916943118:AAH66weDS-domM749NaVhE_jqdrv65vupq0"


async def main():
    bot = Bot(token=TOKEN)

    try:
        me = await bot.get_me()
        print("✅ Токен рабочий!")
        print(f"   Бот: @{me.username}")
        print(f"   Имя: {me.first_name}")
        print(f"   ID: {me.id}")

        # Проверка получения обновлений
        updates = await bot.get_updates()
        print(f"   Очередь обновлений: {len(updates)}")

        return True
    except Exception as e:
        print(f"❌ Ошибка: {e}")
        return False
    finally:
        await bot.session.close()


asyncio.run(main())