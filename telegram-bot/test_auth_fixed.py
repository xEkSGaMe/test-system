# telegram-bot/test_auth_fixed.py
import asyncio
import aiohttp
import re


async def test_auth():
    print("🔍 Тестируем обновлённый Auth API...")

    async with aiohttp.ClientSession() as session:
        # 1. Яндекс OAuth
        print("\n1. Яндекс OAuth:")
        async with session.get(
                'http://localhost:8081/auth/yandex/login',
                allow_redirects=False
        ) as resp:
            print(f"   Статус: {resp.status}")
            print(f"   Заголовки: {dict(resp.headers)}")

            if resp.status == 302:
                location = resp.headers.get('Location')
                print(f"   🔗 Редирект на: {location}")
            else:
                text = await resp.text()
                print(f"   Текст: {text[:200]}...")
                if '<a href="' in text:
                    match = re.search(r'href="([^"]+)"', text)
                    if match:
                        print(f"   🔗 Ссылка в HTML: {match.group(1)}")

        # 2. GitHub OAuth
        print("\n2. GitHub OAuth:")
        async with session.get(
                'http://localhost:8081/auth/github/login',
                allow_redirects=False
        ) as resp:
            print(f"   Статус: {resp.status}")
            if resp.status == 302:
                location = resp.headers.get('Location')
                print(f"   🔗 Редирект на: {location}")

        # 3. Health check
        print("\n3. Health check:")
        async with session.get('http://localhost:8081/health') as resp:
            print(f"   Ответ: {await resp.text()}")

    print("\n✅ Тест завершен!")


if __name__ == "__main__":
    asyncio.run(test_auth())