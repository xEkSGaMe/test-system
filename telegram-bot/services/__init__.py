# telegram-bot/services/__init__.py
from .redis_client import redis_client
from .api_client import api_client

__all__ = ['redis_client', 'api_client']