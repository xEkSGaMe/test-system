# telegram-bot/services/redis_client.py
import json
import redis.asyncio as redis
from config import Config


class RedisClient:
    """Реальный Redis клиент для хранения сессий"""

    def __init__(self):
        self.redis = redis.Redis(
            host=Config.REDIS_HOST,
            port=Config.REDIS_PORT,
            db=Config.REDIS_DB,
            decode_responses=True,
            socket_connect_timeout=5,
            socket_keepalive=True
        )

    async def ping(self) -> bool:
        """Проверка соединения с Redis"""
        try:
            return await self.redis.ping()
        except Exception:
            return False

    async def set_user_session(self, user_id: int, data: dict) -> bool:
        """Сохраняет сессию пользователя"""
        key = f"user:{user_id}:session"
        try:
            await self.redis.setex(
                key,
                Config.SESSION_TTL,
                json.dumps(data)
            )
            return True
        except Exception as e:
            print(f"❌ Ошибка сохранения сессии: {e}")
            return False

    async def get_user_session(self, user_id: int) -> dict | None:
        """Получает сессию пользователя"""
        key = f"user:{user_id}:session"
        try:
            data = await self.redis.get(key)
            if data:
                return json.loads(data)
        except Exception as e:
            print(f"❌ Ошибка получения сессии: {e}")
        return None

    async def delete_user_session(self, user_id: int) -> bool:
        """Удаляет сессию пользователя"""
        key = f"user:{user_id}:session"
        try:
            await self.redis.delete(key)
            return True
        except Exception as e:
            print(f"❌ Ошибка удаления сессии: {e}")
            return False

    async def set_access_token(self, user_id: int, token: str) -> bool:
        """Сохраняет access token пользователя"""
        key = f"user:{user_id}:access_token"
        try:
            await self.redis.setex(
                key,
                Config.SESSION_TTL,
                token
            )
            return True
        except Exception as e:
            print(f"❌ Ошибка сохранения токена: {e}")
            return False

    async def get_access_token(self, user_id: int) -> str | None:
        """Получает access token пользователя"""
        key = f"user:{user_id}:access_token"
        try:
            return await self.redis.get(key)
        except Exception:
            return None

    async def close(self):
        """Закрывает соединение"""
        await self.redis.close()


# Глобальный экземпляр
redis_client = RedisClient()