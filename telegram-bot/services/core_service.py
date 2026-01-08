# telegram-bot/services/core_service.py
import aiohttp
import logging
from config import Config

logger = logging.getLogger(__name__)


class CoreService:
    def __init__(self):
        self.base_url = Config.CORE_API_URL.rstrip('/')

    async def get_tests(self, access_token: str) -> list:
        """Получить список тестов"""
        try:
            headers = {"Authorization": f"Bearer {access_token}"}
            async with aiohttp.ClientSession() as session:
                async with session.get(
                        f"{self.base_url}/tests",
                        headers=headers
                ) as response:
                    if response.status == 200:
                        result = await response.json()
                        # Если API возвращает {"data": [...]}, берем список из data
                        if isinstance(result, dict):
                            return result.get("data", [])
                        # Если API возвращает сразу список [...]
                        return result if isinstance(result, list) else []
                    
                    logger.error(f"Core API ошибка {response.status}: {await response.text()}")
        except Exception as e:
            logger.error(f"Ошибка получения тестов: {e}")
        return []

    async def get_test(self, test_id: int, access_token: str) -> dict:
        """Получить конкретный тест с вопросами"""
        try:
            headers = {"Authorization": f"Bearer {access_token}"}
            async with aiohttp.ClientSession() as session:
                async with session.get(
                        f"{self.base_url}/tests/{test_id}",
                        headers=headers
                ) as response:
                    if response.status == 200:
                        return await response.json()
        except Exception as e:
            logger.error(f"Ошибка получения теста {test_id}: {e}")
        return {}

    async def submit_test(self, test_id: int, answers: list, access_token: str) -> dict:
        """Отправить ответы на тест"""
        try:
            headers = {
                "Authorization": f"Bearer {access_token}",
                "Content-Type": "application/json"
            }
            data = {"answers": answers}

            async with aiohttp.ClientSession() as session:
                async with session.post(
                        f"{self.base_url}/tests/{test_id}/submit",
                        headers=headers,
                        json=data
                ) as response:
                    if response.status == 200:
                        return await response.json()
        except Exception as e:
            logger.error(f"Ошибка отправки теста {test_id}: {e}")
        return {}


core_service = CoreService()