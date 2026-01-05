# check_callbacks.py
import re

def extract_callbacks_from_file(filename):
    """Извлекает все callback_data из файла"""
    with open(filename, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Ищем все callback_data в коде
    patterns = [
        r'callback_data="([^"]+)"',  # callback_data="..."
        r"callback_data='([^']+)'",  # callback_data='...'
        r'F\.data == "([^"]+)"',     # F.data == "..."
        r"F\.data == '([^']+)'",     # F.data == '...'
        r'F\.data\.startswith\("([^"]+)"',  # F.data.startswith("...")
    ]
    
    callbacks = set()
    
    for pattern in patterns:
        matches = re.findall(pattern, content)
        for match in matches:
            callbacks.add(match)
    
    # Ищем @router.callback_query
    handler_pattern = r'@router\.callback_query\(([^)]+)\)'
    handlers = re.findall(handler_pattern, content, re.DOTALL)
    
    print(f"=== АНАЛИЗ {filename} ===")
    print(f"📋 Найдено уникальных callback_data: {len(callbacks)}")
    print("Список:")
    for cb in sorted(callbacks):
        print(f"  • {cb}")
    
    print(f"\n📋 Обработчики (@router.callback_query): {len(handlers)}")
    for i, handler in enumerate(handlers[:5], 1):
        print(f"\n{i}. {handler[:100]}...")
    
    return callbacks

# Проверяем файл tests.py
extract_callbacks_from_file("handlers/tests.py")