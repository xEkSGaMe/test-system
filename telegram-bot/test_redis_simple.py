# telegram-bot/test_redis_simple.py
import asyncio
import redis.asyncio as redis

async def test():
    print("🔍 Тестируем подключение к Redis (localhost:6379)...")
    
    try:
        # Подключаемся к Redis проекта
        r = redis.Redis(host='localhost', port=6379, db=0)
        
        # Пробуем подключиться
        result = await r.ping()
        if result:
            print("✅ Redis подключен успешно!")
        else:
            print("❌ Redis не ответил на PING")
            
        await r.close()
    except Exception as e:
        print(f"❌ Ошибка подключения к Redis: {e}")
        print("   Проверь: docker ps | findstr redis")
        print("   Redis должен быть на порту 6379")

asyncio.run(test())