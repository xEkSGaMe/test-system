import sqlite3
import os

print("=== СОЗДАНИЕ БАЗЫ ДАННЫХ ===")

# Путь к файлу БД
db_path = "test_results.db"
print(f"Создаю файл: {db_path}")

# Подключаемся/создаем БД
conn = sqlite3.connect(db_path)
cursor = conn.cursor()

# Создаем таблицу
cursor.execute('''
CREATE TABLE IF NOT EXISTS test_results (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    user_name TEXT,
    test_id INTEGER NOT NULL,
    test_name TEXT NOT NULL,
    score REAL NOT NULL,
    total_questions INTEGER NOT NULL,
    correct_answers INTEGER NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
)
''')

print("✅ Таблица 'test_results' создана")

# Проверяем
cursor.execute("SELECT name FROM sqlite_master WHERE type='table'")
tables = cursor.fetchall()
print(f"📋 Таблицы в БД: {tables}")

# Добавляем тестовую запись
cursor.execute('''
INSERT INTO test_results 
(user_id, user_name, test_id, test_name, score, total_questions, correct_answers)
VALUES (?, ?, ?, ?, ?, ?, ?)
''', (123456, "Test User", 1, "Python тест", 85.5, 10, 8))

conn.commit()
conn.close()

print("✅ Тестовая запись добавлена")
print(f"📏 Размер файла: {os.path.getsize(db_path)} байт")
print("=== ГОТОВО ===")