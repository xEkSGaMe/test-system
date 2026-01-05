# check_db.py
import sqlite3
import os

print("=== ПРОВЕРКА БАЗЫ ДАННЫХ ===")

db_file = "test_results.db"

if not os.path.exists(db_file):
    print(f"❌ Файл {db_file} не найден!")
    print("Запустите бота и пройдите тест для создания БД.")
else:
    print(f"✅ Файл найден: {db_file}")
    print(f"📏 Размер: {os.path.getsize(db_file)} байт")
    
    try:
        conn = sqlite3.connect(db_file)
        cursor = conn.cursor()
        
        # Проверяем таблицы
        cursor.execute("SELECT name FROM sqlite_master WHERE type='table'")
        tables = cursor.fetchall()
        print(f"📋 Таблицы: {tables}")
        
        # Проверяем test_results
        if 'test_results' in [t[0] for t in tables]:
            cursor.execute("SELECT COUNT(*) FROM test_results")
            count = cursor.fetchone()[0]
            print(f"📊 Записей в test_results: {count}")
            
            if count > 0:
                cursor.execute('''
                SELECT id, user_id, test_name, score, correct_answers, total_questions, created_at 
                FROM test_results 
                ORDER BY id DESC LIMIT 5
                ''')
                records = cursor.fetchall()
                print("\n📝 Последние записи:")
                for row in records:
                    print(f"  ID:{row[0]} User:{row[1]} Test:'{row[2]}' Score:{row[3]}% ({row[4]}/{row[5]}) Date:{row[6][:10]}")
        else:
            print("❌ Таблица 'test_results' не найдена!")
            
        conn.close()
    except Exception as e:
        print(f"❌ Ошибка: {e}")