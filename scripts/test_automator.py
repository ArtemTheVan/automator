"""
test_automator.py - Пример использования библиотеки automator
Использует глобальный экземпляр automator для наглядности.
"""

import time
from wrapper_automator import automator, MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP

def test_basic_operations():
    """Тест базовых операций автоматизации."""
    
    automator.info("Тест 1: Ввод текста")
    automator.info("  Ввод текста 'Hello World!'")
    time.sleep(2)  # Пауза для переключения в нужное окно
    automator.keystroke("Hello World!")
    
    automator.info("Тест 2: Клик мыши")
    automator.info("  Клик в точке (500, 300)")
    time.sleep(1)
    automator.click_at(500, 300)
    
    automator.info("Тест 3: Клик правой кнопкой")
    automator.info("  Клик правой кнопкой в точке (600, 400)")
    time.sleep(1)
    automator.click(600, 400, button='right')
    
    automator.info("Тест 4: Перетаскивание")
    automator.info("  Перетаскивание из (100, 100) в (200, 200)")
    time.sleep(1)
    automator.drag(100, 100, 200, 200)
    
    automator.info("Тест 5: Рисование прямоугольника")
    automator.info("  Рисование прямоугольника 100x100 в (300, 300)")
    time.sleep(1)
    automator.rectangle(300, 300, 100, 100)

def test_advanced_operations():
    """Тест расширенных операций."""
    
    automator.info("Тест 6: Ручная последовательность действий мыши")
    automator.info("  Сложная траектория движения")
    time.sleep(1)
    
    # Создаем сложную траекторию
    actions = [
        (400, 400, MOUSEEVENTF_LEFTDOWN),      # Нажатие
        (450, 400, 0),                          # Вправо
        (450, 450, 0),                          # Вниз
        (400, 450, 0),                          # Влево
        (400, 400, 0),                          # Вверх
        (400, 400, MOUSEEVENTF_LEFTUP),        # Отпускание
    ]
    
    automator.mouse_sequence(actions)
    
    automator.info("Тест 7: Скриншот")
    automator.info("  Захват области экрана")
    time.sleep(1)
    
    # Захват области экрана (например, 800x600 пикселей)
    # Примечание: в реальном использовании укажите правильные координаты
    result = automator.capture_screen(0, 0, 800, 600, "screenshot.png")
    if result == 0:
        automator.info("  ✓ Скриншот сохранен как screenshot.png")
    else:
        automator.error("  ✗ Ошибка при захвате скриншота")

def test_custom_scenario():
    """Тест пользовательского сценария."""
    
    automator.info("\nПользовательский сценарий: Автоматизация Блокнота")
    automator.info("=" * 50)
    
    automator.info("Шаг 1: Открываем Блокнот (Win + R)")
    automator.sleep(1)
    automator.keystroke("notepad")
    automator.sleep(0.5)
    automator.keystroke("\n")  # Enter
    automator.sleep(2)    # Ждем открытия
    
    automator.info("Шаг 2: Вводим текст")
    automator.keystroke("Это автоматически сгенерированный текст!")
    automator.keystroke("\n")
    automator.keystroke("Создано с помощью automator библиотеки.")
    automator.keystroke("\n\n")
    
    automator.info("Шаг 3: Сохраняем файл (Ctrl + S)")
    automator.keystroke("\x13")  # Ctrl+S (0x13 = 19)
    automator.sleep(1)
    
    automator.info("Шаг 4: Вводим имя файла")
    automator.keystroke("automated_file.txt")
    automator.sleep(0.5)
    
    automator.info("Шаг 5: Нажимаем Enter для сохранения")
    automator.keystroke("\n")
    
    automator.info("Шаг 6: Закрываем Блокнот (Alt + F4)")
    automator.sleep(1)
    automator.keystroke("\x00\xF4")  # Alt+F4
    
    automator.info("\n✓ Сценарий завершен!")

def main():
    """Основная функция тестирования."""
    
    print("=" * 60)
    print("Тестирование библиотеки automator")
    print("Используется глобальный экземпляр 'automator'")
    print("=" * 60)
    print(f"\nГлобальный экземпляр: {automator}")
    print(f"Библиотека загружена: {automator.lib is not None}")
    print("\nВнимание! Через 5 секунд начнется выполнение тестов.")
    print("Убедитесь, что у вас есть активное окно для тестирования.")
    print("Для прерывания тестов нажмите Ctrl+C в консоли.")
    print("-" * 60)
    
    try:
        # Обратный отсчет
        for i in range(5, 0, -1):
            print(f"Начало через {i}...", end="\r")
            automator.sleep(1)
        print("\n")
        
        # Запуск тестов
        test_basic_operations()
        
        # Спросим пользователя, продолжать ли
        print("\n" + "-" * 60)
        choice = input("Продолжить тестирование? (y/n): ").lower()
        
        if choice == 'y':
            test_advanced_operations()
            
            print("\n" + "-" * 60)
            choice = input("Запустить пользовательский сценарий? (y/n): ").lower()
            
            if choice == 'y':
                test_custom_scenario()
        
        print("\n" + "=" * 60)
        automator.info("Тестирование завершено!")
        print("=" * 60)
        
    except KeyboardInterrupt:
        automator.error("\n\nТестирование прервано пользователем")
    except Exception as e:
        automator.error(f"\n\nОшибка во время тестирования: {e}")
        print("\nПроверьте:")
        print("1. Установлен ли Python 3.x")
        print("2. Находится ли libautomator.dll в доступном месте")
        print("3. Есть ли активное окно для тестирования")

if __name__ == "__main__":
    main()