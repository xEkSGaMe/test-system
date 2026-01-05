# telegram-bot/services/api_client.py - ИСПРАВЛЕННАЯ ВЕРСИЯ
import aiohttp
import logging
import html  # ⬅️ ДОБАВЬТЕ ЭТОТ ИМПОРТ
import re
import urllib.parse
from config import Config

logger = logging.getLogger(__name__)


class APIClient:
    """Клиент для работы с Auth API"""

    def __init__(self):
        self.base_url = Config.AUTH_API_URL.rstrip('/')
        self.session = None

    async def _get_session(self):
        """Создаёт сессию aiohttp"""
        if self.session is None or self.session.closed:
            self.session = aiohttp.ClientSession()
        return self.session

    async def close(self):
        """Закрывает сессию"""
        if self.session and not self.session.closed:
            await self.session.close()

    def _clean_url(self, raw_url: str) -> str:
        """Очищает URL от HTML-сущностей и URL-encoding"""
        if not raw_url:
            return ""

        # 1. Декодируем HTML-сущности (&amp; → &, &lt; → <)
        cleaned = html.unescape(raw_url)

        # 2. 🔥 ДЕКОДИРУЕМ URL-encoded символы (%3A → :, %2F → /)
        cleaned = urllib.parse.unquote(cleaned)

        # 3. Убираем лишние пробелы и переносы
        cleaned = cleaned.strip()

        # 4. Проверяем что это валидный URL
        if cleaned.startswith("http"):
            return cleaned
        else:
            logger.warning(f"Некорректный URL после очистки: {cleaned[:100]}")
            return raw_url  # Возвращаем оригинал если что-то пошло не так

    async def get_yandex_auth_url(self) -> str | None:
        """Получает ссылку для авторизации через Яндекс"""
        try:
            session = await self._get_session()

            async with session.get(
                    f"{self.base_url}/auth/yandex/login",
                    allow_redirects=False  # ⚠️ НЕ следовать за редиректом!
            ) as response:

                # 🔴 Способ 1: Если это 302 редирект
                if response.status in [302, 303, 307]:
                    location = response.headers.get('Location')
                    if location:
                        logger.info(f"Получен редирект Яндекс: {location[:80]}...")
                        return self._clean_url(location)

                # 🔴 Способ 2: Если это HTML ссылка
                text = await response.text()
                logger.debug(f"Ответ Яндекс (первые 500 символов): {text[:500]}")

                # Ищем ссылку в href
                match = re.search(r'href=["\']([^"\']+)["\']', text)
                if match:
                    raw_url = match.group(1)
                    logger.info(f"Найдена ссылка в HTML: {raw_url[:80]}...")
                    return self._clean_url(raw_url)

                # 🔴 Способ 3: Если ответ просто текст со ссылкой
                match = re.search(r'(https?://[^\s<>"\']+)', text)
                if match:
                    raw_url = match.group(1)
                    logger.info(f"Найдена ссылка в тексте: {raw_url[:80]}...")
                    return self._clean_url(raw_url)

                logger.warning(f"Не удалось найти ссылку в ответе. Статус: {response.status}")
                return None

        except Exception as e:
            logger.error(f"Ошибка получения Яндекс ссылки: {e}", exc_info=True)
        return None

    async def get_github_auth_url(self) -> str | None:
        """Получает ссылку для авторизации через GitHub"""
        try:
            session = await self._get_session()

            async with session.get(
                    f"{self.base_url}/auth/github/login",
                    allow_redirects=False
            ) as response:

                if response.status in [302, 303, 307]:
                    location = response.headers.get('Location')
                    if location:
                        logger.info(f"Получен редирект GitHub: {location[:80]}...")
                        return self._clean_url(location)

                text = await response.text()
                logger.debug(f"Ответ GitHub (первые 500 символов): {text[:500]}")

                match = re.search(r'href=["\']([^"\']+)["\']', text)
                if match:
                    raw_url = match.group(1)
                    logger.info(f"Найдена ссылка в HTML: {raw_url[:80]}...")
                    return self._clean_url(raw_url)

                match = re.search(r'(https?://[^\s<>"\']+)', text)
                if match:
                    raw_url = match.group(1)
                    return self._clean_url(raw_url)

                logger.warning(f"Не удалось найти ссылку GitHub. Статус: {response.status}")
                return None

        except Exception as e:
            logger.error(f"Ошибка получения GitHub ссылки: {e}", exc_info=True)
        return None

    async def validate_token(self, token: str) -> dict:
        """Проверяет валидность JWT токена"""
        try:
            session = await self._get_session()
            headers = {"Authorization": f"Bearer {token}"}

            # 🔴 ИСПРАВЬТЕ ENDPOINT! Должно быть /auth/validate
            async with session.post(
                    f"{self.base_url}/auth/validate",  # ⬅️ ДОБАВЬТЕ /auth/
                    headers=headers
            ) as response:
                if response.status == 200:
                    return await response.json()
                else:
                    logger.error(f"Ошибка валидации: {response.status}")
                    return {"valid": False, "error": f"Status: {response.status}"}
        except Exception as e:
            logger.error(f"Ошибка проверки токена: {e}")
            return {"valid": False, "error": str(e)}

    async def get_user_profile(self, token: str) -> dict:
        """Получает профиль пользователя"""
        try:
            session = await self._get_session()
            headers = {"Authorization": f"Bearer {token}"}

            # 🔴 ИСПРАВЬТЕ ENDPOINT! Должно быть /auth/me
            async with session.get(
                    f"{self.base_url}/auth/me",  # ⬅️ ДОБАВЬТЕ /auth/
                    headers=headers
            ) as response:
                if response.status == 200:
                    return await response.json()
                else:
                    logger.error(f"Ошибка профиля: {response.status}")
                    return {"error": f"Status: {response.status}"}
        except Exception as e:
            logger.error(f"Ошибка получения профиля: {e}")
            return {"error": str(e)}

    async def refresh_token(self, refresh_token: str) -> dict:
        """Обновляет access token"""
        try:
            session = await self._get_session()
            data = {"refresh_token": refresh_token}

            # 🔴 ИСПРАВЬТЕ ENDPOINT! Должно быть /auth/refresh
            async with session.post(
                    f"{self.base_url}/auth/refresh",  # ⬅️ ДОБАВЬТЕ /auth/
                    json=data
            ) as response:
                if response.status == 200:
                    return await response.json()
                else:
                    logger.error(f"Ошибка refresh: {response.status}")
                    return {"error": f"Status: {response.status}"}
        except Exception as e:
            logger.error(f"Ошибка обновления токена: {e}")
            return {"error": str(e)}


# Глобальный экземпляр
api_client = APIClient()