import sys
import os
import time
from datetime import datetime

def main():
    print("=" * 60)
    print("Python скрипт выполняется из Qt приложения")
    print(f"Время запуска: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print("=" * 60)
    
    # Проверяем наличие DLL
    dll_path = "..lib/libautomator.dll"
    if os.path.exists(dll_path):
        print(f"Библиотека automator найдена: {dll_path}")
        
        try:
            # Пытаемся загрузить библиотеку
            import ctypes
            automator = ctypes.WinDLL(dll_path)
            
            # Настраиваем функции
            automator.simulate_keystroke.argtypes = [ctypes.c_char_p]
            automator.simulate_keystroke.restype = None
            
            automator.simulate_mouse_click_at.argtypes = [ctypes.c_int, ctypes.c_int]
            automator.simulate_mouse_click_at.restype = None
            
            print("Библиотека automator успешно загружена")
            
            # Пример использования
            print("Выполняю тестовые действия...")
            
            # Пауза перед действиями
            time.sleep(2)
            
            # Ввод текста
            print("Ввод текста: 'Hello from Python!'")
            # automator.simulate_keystroke(b"Hello from Python!")
            
            # Клик мыши
            print("Клик мыши в точке (500, 300)")
            # automator.simulate_mouse_click_at(500, 300)
            
            # Еще пауза
            time.sleep(1)
            
        except Exception as e:
            print(f"Ошибка при работе с библиотекой: {e}")
    else:
        print(f"Библиотека automator не найдена по пути: {dll_path}")
        print("Убедитесь, что libautomator.dll находится в той же папке")
    
    print("\n" + "=" * 60)
    print("Скрипт завершен успешно!")
    print("=" * 60)

if __name__ == "__main__":
    main()