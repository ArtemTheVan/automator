"""
qt_example_script.py - Пример скрипта для выполнения в Qt приложении
"""

from wrapper_automator import automator
import time

def main():
    """Основная функция скрипта для Qt."""
    
    automator.info("Начинаю выполнение скрипта из Qt приложения...")
    
    # Пауза для подготовки
    automator.sleep(3)
    
    # Простая демонстрация
    automator.keystroke("Hello from Qt Python Script!")
    automator.sleep(1)
    
    # Клик в центре экрана (примерные координаты)
    automator.click(960, 540)
    automator.sleep(1)
    
    # Ввод еще текста
    automator.keystroke("\nЭто автоматизированное сообщение.")
    
    automator.info("Скрипт успешно выполнен!")

if __name__ == "__main__":
    main()