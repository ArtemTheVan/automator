"""
wrapper_automator.py - Обертка для библиотеки automator.dll
Предоставляет удобный интерфейс для работы с функциями автоматизации.
"""

import ctypes
from ctypes import c_int, c_char_p, c_void_p, Structure, POINTER, byref
import os
import sys
import time

# =========== Константы ===========

# Константы для флагов мыши (значения из WinAPI)
MOUSEEVENTF_MOVE = 0x0001
MOUSEEVENTF_LEFTDOWN = 0x0002
MOUSEEVENTF_LEFTUP = 0x0004
MOUSEEVENTF_RIGHTDOWN = 0x0008
MOUSEEVENTF_RIGHTUP = 0x0010
MOUSEEVENTF_MIDDLEDOWN = 0x0020
MOUSEEVENTF_MIDDLEUP = 0x0040
MOUSEEVENTF_ABSOLUTE = 0x8000

# Коды клавиш для специальных клавиш (дополнительно)
VK_SHIFT = 0x10
VK_CONTROL = 0x11
VK_ALT = 0x12
VK_ENTER = 0x0D
VK_TAB = 0x09
VK_ESCAPE = 0x1B
VK_SPACE = 0x20
VK_BACK = 0x08
VK_DELETE = 0x2E
VK_HOME = 0x24
VK_END = 0x23
VK_PAGEUP = 0x21
VK_PAGEDOWN = 0x22
VK_UP = 0x26
VK_DOWN = 0x28
VK_LEFT = 0x25
VK_RIGHT = 0x27

# =========== Структуры ===========

class MouseAction(Structure):
    """
    Структура для описания одного действия мыши.
    Аналогична структуре в C-библиотеке automator.
    """
    _fields_ = [
        ("x", c_int),              # Координата X
        ("y", c_int),              # Координата Y  
        ("dwFlags", ctypes.c_ulong)  # Флаги действия (DWORD)
    ]

# =========== Основной класс обертки ===========

class Automator:
    """
    Основной класс для работы с библиотекой automator.dll.
    Создает глобальный экземпляр для использования во всех скриптах.
    """
    
    # Глобальный экземпляр (синглтон)
    _instance = None
    _initialized = False
    
    def __new__(cls, dll_path=None):
        """Паттерн синглтон - возвращает один экземпляр на все приложение."""
        if cls._instance is None:
            cls._instance = super(Automator, cls).__new__(cls)
        return cls._instance
    
    def __init__(self, dll_path=None):
        """
        Инициализация обертки. Загружает библиотеку automator.dll.
        
        Args:
            dll_path (str, optional): Путь к библиотеке. Если None, 
                                     пытается найти в стандартных местах.
        """
        # Инициализируем только один раз
        if self._initialized:
            return
            
        self.lib = None
        self._dll_path = dll_path
        self._load_library()
        self._setup_functions()
        self._initialized = True
        
        print(f"[Automator] Библиотека загружена успешно")
    
    def _load_library(self):
        """
        Загружает библиотеку automator.dll.
        """
        # Список возможных путей к библиотеке
        possible_paths = []
        
        # Если указан явный путь
        if self._dll_path:
            possible_paths.append(self._dll_path)
        
        # Добавляем стандартные пути
        possible_paths.extend([
            "libautomator.dll",                    # Текущая директория
            "./libautomator.dll",                  # Текущая директория
            "../lib/libautomator.dll",             # Относительный путь
            "../../lib/libautomator.dll",          # Относительный путь
            "D:/Projects/automator/lib/libautomator.dll",  # Абсолютный путь
            "C:/Projects/automator/lib/libautomator.dll",  # Абсолютный путь
        ])
        
        # Добавляем путь из переменной окружения (если есть)
        if 'AUTOMATOR_DLL_PATH' in os.environ:
            possible_paths.append(os.environ['AUTOMATOR_DLL_PATH'])
        
        # Пробуем загрузить библиотеку
        for path in possible_paths:
            try:
                self.lib = ctypes.WinDLL(path)
                print(f"[Automator] Библиотека загружена из: {path}")
                return
            except (OSError, FileNotFoundError) as e:
                continue
        
        # Если библиотека не найдена
        raise FileNotFoundError(
            "[Automator] Не удалось найти или загрузить библиотеку libautomator.dll.\n"
            "Убедитесь, что она находится в одном из следующих мест:\n" +
            "\n".join(possible_paths)
        )
    
    def _setup_functions(self):
        """
        Настраивает типы аргументов и возвращаемых значений для функций библиотеки.
        """
        if not self.lib:
            raise RuntimeError("[Automator] Библиотека не загружена")
        
        # simulate_keystroke(const char *text)
        self.lib.simulate_keystroke.argtypes = [c_char_p]
        self.lib.simulate_keystroke.restype = None
        
        # simulate_mouse_sequence(const MouseAction *actions, int count)
        self.lib.simulate_mouse_sequence.argtypes = [POINTER(MouseAction), c_int]
        self.lib.simulate_mouse_sequence.restype = None
        
        # simulate_mouse_click_at(int x, int y)
        self.lib.simulate_mouse_click_at.argtypes = [c_int, c_int]
        self.lib.simulate_mouse_click_at.restype = None
        
        # capture_screen_region(int x, int y, int w, int h, const char* filename)
        self.lib.capture_screen_region.argtypes = [c_int, c_int, c_int, c_int, c_char_p]
        self.lib.capture_screen_region.restype = c_int
    
    # =========== Основные методы ===========
    
    def keystroke(self, text):
        """
        Симуляция нажатия клавиш для ввода текста.
        
        Args:
            text (str): Текст для ввода
        """
        if isinstance(text, str):
            text = text.encode('utf-8')
        self.lib.simulate_keystroke(text)
    
    def mouse_sequence(self, actions):
        """
        Эмуляция последовательности действий мыши.
        
        Args:
            actions: Список кортежей (x, y, flags) или словарей
        """
        count = len(actions)
        actions_array = (MouseAction * count)()
        
        for i, action in enumerate(actions):
            if isinstance(action, dict):
                actions_array[i].x = action['x']
                actions_array[i].y = action['y']
                actions_array[i].dwFlags = action.get('flags', 0)
            else:
                # Кортеж (x, y, flags)
                actions_array[i].x = action[0]
                actions_array[i].y = action[1]
                actions_array[i].dwFlags = action[2] if len(action) > 2 else 0
        
        self.lib.simulate_mouse_sequence(actions_array, count)
    
    def click_at(self, x, y):
        """
        Клик мыши в указанных координатах.
        
        Args:
            x (int): Координата X
            y (int): Координата Y
        """
        self.lib.simulate_mouse_click_at(x, y)
    
    def capture_screen(self, x, y, width, height, filename):
        """
        Захват области экрана.
        
        Args:
            x (int): Координата X левого верхнего угла
            y (int): Координата Y левого верхнего угла
            width (int): Ширина области
            height (int): Высота области
            filename (str): Имя файла для сохранения
            
        Returns:
            int: Код результата (0 - успех)
        """
        if isinstance(filename, str):
            filename = filename.encode('utf-8')
        return self.lib.capture_screen_region(x, y, width, height, filename)
    
    # =========== Вспомогательные методы ===========
    
    def click(self, x, y, button='left'):
        """
        Клик указанной кнопкой мыши в координатах.
        
        Args:
            x (int): Координата X
            y (int): Координата Y
            button (str): Кнопка мыши ('left', 'right', 'middle')
        """
        flags_down = MOUSEEVENTF_LEFTDOWN
        flags_up = MOUSEEVENTF_LEFTUP
        
        if button == 'right':
            flags_down = MOUSEEVENTF_RIGHTDOWN
            flags_up = MOUSEEVENTF_RIGHTUP
        elif button == 'middle':
            flags_down = MOUSEEVENTF_MIDDLEDOWN
            flags_up = MOUSEEVENTF_MIDDLEUP
        
        actions = [(x, y, flags_down), (x, y, flags_up)]
        self.mouse_sequence(actions)
    
    def drag(self, start_x, start_y, end_x, end_y, button='left'):
        """
        Перетаскивание мышью из одной точки в другую.
        
        Args:
            start_x (int): Начальная координата X
            start_y (int): Начальная координата Y
            end_x (int): Конечная координата X
            end_y (int): Конечная координата Y
            button (str): Кнопка мыши ('left', 'right', 'middle')
        """
        flags_down = MOUSEEVENTF_LEFTDOWN
        flags_up = MOUSEEVENTF_LEFTUP
        
        if button == 'right':
            flags_down = MOUSEEVENTF_RIGHTDOWN
            flags_up = MOUSEEVENTF_RIGHTUP
        elif button == 'middle':
            flags_down = MOUSEEVENTF_MIDDLEDOWN
            flags_up = MOUSEEVENTF_MIDDLEUP
        
        actions = [
            (start_x, start_y, flags_down),  # Нажатие кнопки
            (end_x, end_y, 0),               # Перемещение с зажатой кнопкой
            (end_x, end_y, flags_up)         # Отпускание кнопки
        ]
        self.mouse_sequence(actions)
    
    def move(self, x, y, absolute=True):
        """
        Перемещение мыши в указанные координаты.
        
        Args:
            x (int): Координата X
            y (int): Координата Y
            absolute (bool): Использовать абсолютные координаты
        """
        flags = MOUSEEVENTF_MOVE
        if absolute:
            flags |= MOUSEEVENTF_ABSOLUTE
        self.mouse_sequence([(x, y, flags)])
    
    def rectangle(self, x, y, width, height):
        """
        Рисование прямоугольника мышью.
        
        Args:
            x (int): Координата X левого верхнего угла
            y (int): Координата Y левого верхнего угла
            width (int): Ширина прямоугольника
            height (int): Высота прямоугольника
        """
        actions = [
            (x, y, MOUSEEVENTF_LEFTDOWN),              # Нажатие в начальной точке
            (x + width, y, 0),                         # Перемещение вправо
            (x + width, y + height, 0),                # Перемещение вниз
            (x, y + height, 0),                        # Перемещение влево
            (x, y, 0),                                 # Возврат в начало
            (x, y, MOUSEEVENTF_LEFTUP)                 # Отпускание кнопки
        ]
        self.mouse_sequence(actions)
    
    def sleep(self, seconds):
        """
        Пауза в выполнении скрипта.
        
        Args:
            seconds (float): Количество секунд для паузы
        """
        time.sleep(seconds)
    
    def msleep(self, milliseconds):
        """
        Пауза в выполнении скрипта (миллисекунды).
        
        Args:
            milliseconds (int): Количество миллисекунд для паузы
        """
        time.sleep(milliseconds / 1000.0)
    
    def info(self, message):
        """
        Вывод информационного сообщения.
        
        Args:
            message (str): Сообщение для вывода
        """
        print(f"[INFO] {message}")
    
    def warning(self, message):
        """
        Вывод предупреждающего сообщения.
        
        Args:
            message (str): Сообщение для вывода
        """
        print(f"[WARNING] {message}")
    
    def error(self, message):
        """
        Вывод сообщения об ошибке.
        
        Args:
            message (str): Сообщение для вывода
        """
        print(f"[ERROR] {message}")

# =========== Глобальный экземпляр для удобства ===========

# Создаем глобальный экземпляр
automator = Automator()

# =========== Экспорт ===========

__all__ = [
    # Основной класс
    'Automator',
    
    # Глобальный экземпляр
    'automator',
    
    # Структуры
    'MouseAction',
    
    # Константы
    'MOUSEEVENTF_MOVE',
    'MOUSEEVENTF_LEFTDOWN',
    'MOUSEEVENTF_LEFTUP',
    'MOUSEEVENTF_RIGHTDOWN',
    'MOUSEEVENTF_RIGHTUP',
    'MOUSEEVENTF_MIDDLEDOWN',
    'MOUSEEVENTF_MIDDLEUP',
    'MOUSEEVENTF_ABSOLUTE',
    'VK_SHIFT', 'VK_CONTROL', 'VK_ALT', 'VK_ENTER', 'VK_TAB', 'VK_ESCAPE',
    'VK_SPACE', 'VK_BACK', 'VK_DELETE', 'VK_HOME', 'VK_END',
    'VK_PAGEUP', 'VK_PAGEDOWN', 'VK_UP', 'VK_DOWN', 'VK_LEFT', 'VK_RIGHT',
]

# =========== Тест при прямом запуске ===========

if __name__ == "__main__":
    print("Тестирование wrapper_automator.py")
    print("=" * 50)
    
    try:
        print(f"Глобальный экземпляр automator создан: {automator}")
        print(f"Библиотека загружена: {automator.lib is not None}")
        
        # Простой тест констант
        print("\nКонстанты загружены:")
        print(f"  MOUSEEVENTF_LEFTDOWN = {MOUSEEVENTF_LEFTDOWN:#06x}")
        print(f"  VK_ENTER = {VK_ENTER:#04x}")
        
        print("\nДоступные методы automator:")
        methods = [m for m in dir(automator) if not m.startswith('_') and m not in ['lib', 'dll_path']]
        for i, method in enumerate(methods):
            print(f"  {method}", end=", " if (i + 1) % 5 != 0 else "\n")
        
        print("\n\nПример использования в скрипте:")
        print("""
from wrapper_automator import automator
import time

def main():
    automator.info("Начинаю выполнение скрипта...")
    time.sleep(2)
    
    # Ввод текста
    automator.keystroke("Hello World!")
    
    # Клик мыши
    automator.click(500, 300)
    
    automator.info("Скрипт выполнен!")

if __name__ == "__main__":
    main()
        """)
        
    except Exception as e:
        print(f"✗ Ошибка: {e}")
        print("\nУбедитесь, что:")
        print("1. Библиотека libautomator.dll существует")
        print("2. Установлен Python 3.x")
        print("3. Библиотека находится в одном из стандартных путей")