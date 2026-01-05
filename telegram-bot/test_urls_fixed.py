import asyncio
import sys
import os

sys.path.append(os.path.dirname(os.path.dirname(__file__)))

from services.api_client import APIClient


async def test():
    print("🧪 Тестируем ссылки с декодированием URL...\n")

    client = APIClient()

    # Тест Яндекс
    print("1. Яндекс OAuth:")
    yandex = await client.get_yandex_auth_url()
    if yandex:
        print(f"   🔗 {yandex[:120]}")
        print(f"   ✅ Длина: {len(yandex)}")
        print(f"   ✅ Содержит 'oauth.yandex.ru': {'oauth.yandex.ru' in yandex}")
        print(f"   ✅ Без %3A: {':/?' in yandex}")
        print(f"   ✅ Кликабельная: {yandex.startswith('https://')}")
    else:
        print("   ❌ Не получили ссылку")

    print("\n" + "─" * 60 + "\n")

    # Тест GitHub
    print("2. GitHub OAuth:")
    github = await client.get_github_auth_url()
    if github:
        print(f"   🔗 {github[:120]}")
        print(f"   ✅ Длина: {len(github)}")
        print(f"   ✅ Содержит 'github.com': {'github.com' in github}")
        print(f"   ✅ Без %3A: {':/?' in github}")
        print(f"   ✅ Кликабельная: {github.startswith('https://')}")
    else:
        print("   ❌ Не получили ссылку")

    print("\n" + "═" * 60)
    print("📱 ССЫЛКИ ДЛЯ ПРОВЕРКИ В БРАУЗЕРЕ:")
    if yandex:
        print(f"\n🔵 Яндекс:\n{yandex}")
    if github:
        print(f"\n⚫ GitHub:\n{github}")

    await client.close()


if __name__ == "__main__":
    asyncio.run(test())