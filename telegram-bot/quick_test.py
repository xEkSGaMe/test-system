# quick_test.py
import asyncio
import aiohttp


async def test_telegram_api():
    # 🔴 ВСТАВЬ СВОЙ ТОКЕН
    token = "8509864529:ААGyК6m1qVPuPKYZLgdcy@_INjg2@_hTg84"

    url = f"https://api.telegram.org/bot{token}/getMe"

    try:
        async with aiohttp.ClientSession() as session:
            async with session.get(url) as response:
                data = await response.json()

                if data.get("ok"):
                    print(f"✅ Telegram API отвечает")
                    print(f"   Бот: @{data['result']['username']}")
                else:
                    print(f"❌ Ошибка Telegram: {data.get('description')}")

    except Exception as e:
        print(f"❌ Сетевая ошибка: {e}")


asyncio.run(test_telegram_api())