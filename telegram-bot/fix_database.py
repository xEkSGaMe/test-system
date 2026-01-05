# fix_database.py
import sqlite3

print("=== ИСПРАВЛЕНИЕ БАЗЫ ДАННЫХ ===")

conn = sqlite3.connect("test_results.db")
cursor = conn.cursor()

# 1. Посмотрим текущие записи
cursor.execute("SELECT id, user_id, test_name FROM test_results")
records = cursor.fetchall()

print("📋 Текущие записи:")
for record in records:
    print(f"  ID:{record[0]}, user_id:{record[1]}, test:'{record[2]}'")

# 2. Удалим записи с неправильными user_id
cursor.execute("DELETE FROM test_results WHERE user_id != 929595851 AND user_id != ?", (929595851,))
deleted = cursor.rowcount

# 3. Добавим тестовую запись с правильным user_id
cursor.execute('''
INSERT INTO test_results 
(user_id, user_name, test_id, test_name, score, total_questions, correct_answers)
VALUES (?, ?, ?, ?, ?, ?, ?)
''', (929595851, "Тестовый Пользователь", 1, "Python тест", 85.5, 10, 8))

conn.commit()
conn.close()

print(f"🗑️ Удалено {deleted} записей с неправильными user_id")
print("✅ Добавлена тестовая запись для user_id=929595851")
print("=== ГОТОВО ===")