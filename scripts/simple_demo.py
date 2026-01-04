"""
simple_demo.py - Простой демонстрационный скрипт
Использует глобальный экземпляр automator для наглядности.
"""

import time
from wrapper_automator import automator

def main():
    """Основная функция демо-скрипта."""
    
    automator.info("=== Запуск демонстрационного скрипта ===")
    automator.info("Используется глобальный экземпляр 'automator'")
    
    # Пауза перед началом
    automator.info("Пауза 3 секунды перед началом...")
    automator.sleep(3)
    
    # Демонстрация различных методов
    automator.info("1. Ввод текста")
    automator.keystroke("Привет от Python скрипта!")
    automator.sleep(1)
    
    automator.info("2. Клик левой кнопкой мыши")
    automator.click(500, 300)
    automator.sleep(1)
    
    automator.info("3. Перемещение мыши")
    automator.move(600, 400)
    automator.sleep(1)
    
    automator.info("4. Клик правой кнопкой")
    automator.click(700, 350, button='right')
    automator.sleep(1)
    
    automator.info("5. Рисование квадрата")
    automator.rectangle(400, 200, 150, 150)
    automator.sleep(1)
    
    automator.info("6. Перетаскивание")
    automator.drag(200, 200, 300, 300)
    
    automator.info("=== Демонстрация завершена ===")
    automator.info(f"Всего выполнено 6 действий")

if __name__ == "__main__":
    main()