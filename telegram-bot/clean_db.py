# clean_db.py
import sqlite3

print("=== ОЧИСТКА БАЗЫ ДАННЫХ ===")

conn = sqlite3.connect("test_results.db")
cursor = conn.cursor()

# 1. Удаляем все записи
cursor.execute("DELETE FROM test_results")
deleted = cursor.rowcount

# 2. Сбрасываем автоинкремент
cursor.execute("DELETE FROM sqlite_sequence WHERE name='test_results'")

# 3. Добавляем тестовую запись с правильным user_id
cursor.execute('''
INSERT INTO test_results 
(user_id, user_name, test_id, test_name, score, total_questions, correct_answers)
VALUES (?, ?, ?, ?, ?, ?, ?)
''', (929595851, "Тестовый Пользователь", 1, "Python: Основы программирования", 100.0, 2, 2))

conn.commit()
conn.close()

print(f"🗑️ Удалено {deleted} записей")
print("✅ База данных очищена и добавлена тестовая запись")
print("=== ГОТОВО ===")