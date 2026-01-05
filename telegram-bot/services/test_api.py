# services/test_api.py
import aiohttp
import json
from typing import List, Dict, Any
from config import Config


class TestAPI:
    """Клиент для работы с Core API тестовой системы"""

    def __init__(self):
        self.base_url = Config.CORE_API_URL
        self.timeout = aiohttp.ClientTimeout(total=10)

    async def get_all_tests(self) -> List[Dict]:
        """Получить список всех тестов"""
        try:
            async with aiohttp.ClientSession(timeout=self.timeout) as session:
                url = f"{self.base_url}/tests"
                print(f"🌐 Запрос: GET {url}")

                async with session.get(url) as response:
                    if response.status == 200:
                        data = await response.json()
                        print(f"✅ Получено тестов: {len(data)}")
                        return data
                    else:
                        print(f"❌ Ошибка: {response.status}")
                        return []
        except Exception as e:
            print(f"⚠️ Ошибка подключения: {e}")
            return []

    async def get_test_by_id(self, test_id: int) -> Dict:
        """Получить тест с вопросами по ID"""
        try:
            async with aiohttp.ClientSession(timeout=self.timeout) as session:
                url = f"{self.base_url}/tests/{test_id}"
                print(f"🌐 Запрос: GET {url}")

                async with session.get(url) as response:
                    if response.status == 200:
                        data = await response.json()
                        print(f"✅ Получен тест ID {test_id}: {data.get('title', 'Без названия')}")
                        return data
                    else:
                        print(f"❌ Тест не найден: {response.status}")
                        return {}
        except Exception as e:
            print(f"⚠️ Ошибка: {e}")
            return {}

    async def submit_test_answers(self, test_id: int, user_id: int, answers: List[Dict]) -> Dict:
        """Отправить ответы на тест"""
        try:
            async with aiohttp.ClientSession(timeout=self.timeout) as session:
                url = f"{self.base_url}/tests/{test_id}/submit"
                print(f"🌐 Запрос: POST {url}")

                payload = {
                    "user_id": user_id,
                    "answers": answers
                }

                print(f"📦 Отправляемые данные: {json.dumps(payload, ensure_ascii=False)}")

                async with session.post(url, json=payload) as response:
                    if response.status == 200:
                        data = await response.json()
                        print(
                            f"✅ Ответы отправлены: attempt_id={data.get('attempt_id')}, score={data.get('score')}/{data.get('max_score')}")
                        return data
                    else:
                        error_text = await response.text()
                        print(f"❌ Ошибка отправки: {response.status} - {error_text}")
                        return {"error": f"HTTP {response.status}"}
        except Exception as e:
            print(f"⚠️ Ошибка: {e}")
            return {"error": str(e)}

    async def check_connection(self) -> bool:
        """Проверить подключение к API"""
        try:
            async with aiohttp.ClientSession(timeout=aiohttp.ClientTimeout(total=5)) as session:
                async with session.get(f"{self.base_url}/health") as response:
                    return response.status == 200
        except:
            return False