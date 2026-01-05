import sqlite3
import json
from pathlib import Path

DB_PATH = Path(__file__).parent / "test_results.db"


def init_db():
    def init_db():
        """Инициализирует базу данных"""
        print("=== DEBUG: init_db() ВЫЗВАНА ===")
        print(f"Текущая директория: {os.getcwd()}")

        import sqlite3
        import json
        import os

        # Путь к базе данных рядом с bot.py
        DB_PATH = os.path.join(os.path.dirname(os.path.dirname(__file__)), "test_results.db")
        print(f"Путь к БД: {DB_PATH}")

        # Проверяем, существует ли файл
        if os.path.exists(DB_PATH):
            print(f"✅ Файл БД уже существует: {DB_PATH}")
        else:
            print(f"🆕 Файл БД будет создан: {DB_PATH}")

        # ... остальной код функции
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()

    cursor.execute('''
    CREATE TABLE IF NOT EXISTS test_results (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        user_id INTEGER NOT NULL,
        test_id INTEGER NOT NULL,
        test_name TEXT NOT NULL,
        score REAL NOT NULL,
        total_questions INTEGER NOT NULL,
        answers_json TEXT NOT NULL,
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    )
    ''')

    conn.commit()
    conn.close()


def save_result(user_id, test_id, test_name, score, total_questions, answers):
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()

    cursor.execute('''
    INSERT INTO test_results 
    (user_id, test_id, test_name, score, total_questions, answers_json)
    VALUES (?, ?, ?, ?, ?, ?)
    ''', (user_id, test_id, test_name, score, total_questions, json.dumps(answers)))

    conn.commit()
    conn.close()


def get_user_results(user_id):
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()

    cursor.execute('''
    SELECT test_name, score, total_questions, created_at 
    FROM test_results 
    WHERE user_id = ? 
    ORDER BY created_at DESC
    ''', (user_id,))

    return cursor.fetchall()