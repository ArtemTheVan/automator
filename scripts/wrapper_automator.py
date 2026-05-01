"""
wrapper_automator.py - Обертка для библиотеки automator.dll
Предоставляет удобный интерфейс для работы с функциями автоматизации.

Поиск libautomator.dll выполняется в следующем порядке:
    1. Явный путь, переданный в Automator(dll_path=...)
    2. Переменная окружения AUTOMATOR_DLL_PATH
    3. Каталог рядом с этим скриптом (../lib/libautomator.dll)
    4. Каталог рядом с этим скриптом (./libautomator.dll)
    5. Текущая рабочая директория
    6. Системный поиск DLL (PATH)

Каталог %TEMP% намеренно НЕ просматривается — это предотвращает DLL hijacking.
"""

import ctypes
import logging
import os
import sys
import time
from ctypes import POINTER, Structure, c_char_p, c_int

log = logging.getLogger("automator")

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

_BUTTON_FLAGS = {
    "left":   (MOUSEEVENTF_LEFTDOWN,   MOUSEEVENTF_LEFTUP),
    "right":  (MOUSEEVENTF_RIGHTDOWN,  MOUSEEVENTF_RIGHTUP),
    "middle": (MOUSEEVENTF_MIDDLEDOWN, MOUSEEVENTF_MIDDLEUP),
}

# =========== Структуры ===========


class MouseAction(Structure):
    """Структура одного действия мыши, аналог MouseAction из C-библиотеки."""
    _fields_ = [
        ("x", c_int),
        ("y", c_int),
        ("dwFlags", ctypes.c_ulong),
    ]


# =========== Поиск библиотеки ===========


def _candidate_dll_paths(explicit_path):
    """
    Возвращает список кандидатных путей к libautomator.dll в порядке приоритета.

    %TEMP% намеренно исключён — это предотвращает DLL hijacking.
    """
    here = os.path.dirname(os.path.abspath(__file__))
    candidates = []

    if explicit_path:
        candidates.append(explicit_path)

    env_path = os.environ.get("AUTOMATOR_DLL_PATH")
    if env_path:
        candidates.append(env_path)

    candidates.extend([
        os.path.join(here, "..", "lib", "libautomator.dll"),
        os.path.join(here, "libautomator.dll"),
        os.path.join(os.getcwd(), "libautomator.dll"),
        "libautomator.dll",  # системный поиск через PATH
    ])

    return [os.path.normpath(p) for p in candidates]


# =========== Основной класс обертки ===========


class Automator:
    """
    Основной класс для работы с библиотекой automator.dll.
    Реализует паттерн «синглтон» — один экземпляр на процесс.
    """

    _instance = None
    _initialized = False

    def __new__(cls, dll_path=None):
        if cls._instance is None:
            cls._instance = super(Automator, cls).__new__(cls)
        return cls._instance

    def __init__(self, dll_path=None):
        """
        Инициализация обёртки. Загружает libautomator.dll.

        Args:
            dll_path: Явный путь к DLL. Если None — поиск по умолчанию
                      (см. модульный docstring).
        """
        if self._initialized:
            return

        self.lib = None
        self._dll_path = dll_path
        self._load_library()
        self._setup_functions()

        self._initialized = True
        log.info("Automator initialized (DLL: %s)", self._loaded_path)

    def _load_library(self):
        """Загружает libautomator.dll по списку кандидатных путей."""
        last_error = None

        for path in _candidate_dll_paths(self._dll_path):
            # Для системного поиска (одного только имени файла) os.path.exists()
            # вернёт False, но ctypes.CDLL всё равно сможет найти DLL через PATH.
            is_bare_name = os.path.dirname(path) == ""

            if not is_bare_name and not os.path.exists(path):
                log.debug("DLL not found at: %s", path)
                continue

            log.debug("Trying to load DLL: %s", path)

            # Python 3.8+: добавляем КАТАЛОГ DLL (а не сам файл) в пути поиска,
            # чтобы зависимые DLL загрузились корректно.
            if not is_bare_name and hasattr(os, "add_dll_directory"):
                dll_dir = os.path.dirname(os.path.abspath(path))
                if os.path.isdir(dll_dir):
                    try:
                        os.add_dll_directory(dll_dir)
                    except (OSError, FileNotFoundError) as e:
                        log.debug("add_dll_directory(%s) failed: %s", dll_dir, e)

            try:
                self.lib = ctypes.CDLL(path)  # CDLL — для MinGW-сборки
                self._loaded_path = path
                return
            except OSError as e:
                last_error = e
                log.debug("CDLL load failed for %s: %s", path, e)

        raise FileNotFoundError(
            "Не удалось загрузить libautomator.dll. "
            "Передайте путь явно через Automator(dll_path=...) или "
            "укажите переменную окружения AUTOMATOR_DLL_PATH. "
            f"Последняя ошибка: {last_error}"
        )

    def _setup_functions(self):
        """Настраивает argtypes/restype для C-функций."""
        self.lib.simulate_keystroke.argtypes = [c_char_p]
        self.lib.simulate_keystroke.restype = None

        self.lib.simulate_mouse_sequence.argtypes = [POINTER(MouseAction), c_int]
        self.lib.simulate_mouse_sequence.restype = None

        self.lib.simulate_mouse_click_at.argtypes = [c_int, c_int]
        self.lib.simulate_mouse_click_at.restype = None

        self.lib.capture_screen_region.argtypes = [c_int, c_int, c_int, c_int, c_char_p]
        self.lib.capture_screen_region.restype = c_int

    # =========== Базовые методы ===========

    def keystroke(self, text):
        """Симуляция нажатия клавиш для ввода текста (UTF-8)."""
        if isinstance(text, str):
            text = text.encode("utf-8")
        self.lib.simulate_keystroke(text)

    def mouse_sequence(self, actions):
        """
        Эмуляция последовательности действий мыши.

        Args:
            actions: список кортежей (x, y, flags) или словарей
                     {"x": ..., "y": ..., "flags": ...}.
        """
        count = len(actions)
        actions_array = (MouseAction * count)()

        for i, action in enumerate(actions):
            if isinstance(action, dict):
                actions_array[i].x = action["x"]
                actions_array[i].y = action["y"]
                actions_array[i].dwFlags = action.get("flags", 0)
            else:
                actions_array[i].x = action[0]
                actions_array[i].y = action[1]
                actions_array[i].dwFlags = action[2] if len(action) > 2 else 0

        self.lib.simulate_mouse_sequence(actions_array, count)

    def capture_screen(self, x, y, width, height, filename):
        """Захват области экрана. Возвращает 0 при успехе, !=0 при ошибке."""
        if isinstance(filename, str):
            filename = filename.encode("utf-8")
        return self.lib.capture_screen_region(x, y, width, height, filename)

    # =========== Высокоуровневые методы ===========

    def click(self, x, y, button="left"):
        """Клик указанной кнопкой мыши в координатах."""
        if button not in _BUTTON_FLAGS:
            raise ValueError(f"Unknown button: {button!r}")
        flags_down, flags_up = _BUTTON_FLAGS[button]
        self.mouse_sequence([(x, y, flags_down), (x, y, flags_up)])

    # Алиас для обратной совместимости — используйте click().
    click_at = click

    def drag(self, start_x, start_y, end_x, end_y, button="left"):
        """Перетаскивание мышью из одной точки в другую."""
        if button not in _BUTTON_FLAGS:
            raise ValueError(f"Unknown button: {button!r}")
        flags_down, flags_up = _BUTTON_FLAGS[button]
        self.mouse_sequence([
            (start_x, start_y, flags_down),
            (end_x, end_y, 0),
            (end_x, end_y, flags_up),
        ])

    def move(self, x, y, absolute=True):
        """Перемещение мыши в указанные координаты."""
        flags = MOUSEEVENTF_MOVE
        if absolute:
            flags |= MOUSEEVENTF_ABSOLUTE
        self.mouse_sequence([(x, y, flags)])

    def rectangle(self, x, y, width, height):
        """Рисование прямоугольника зажатой левой кнопкой."""
        self.mouse_sequence([
            (x, y, MOUSEEVENTF_LEFTDOWN),
            (x + width, y, 0),
            (x + width, y + height, 0),
            (x, y + height, 0),
            (x, y, 0),
            (x, y, MOUSEEVENTF_LEFTUP),
        ])

    # =========== Утилиты ===========

    def sleep(self, seconds):
        """Пауза в секундах."""
        time.sleep(seconds)

    def msleep(self, milliseconds):
        """Пауза в миллисекундах."""
        time.sleep(milliseconds / 1000.0)

    def info(self, message):
        log.info(message)
        print(f"[INFO] {message}")

    def warning(self, message):
        log.warning(message)
        print(f"[WARNING] {message}")

    def error(self, message):
        log.error(message)
        print(f"[ERROR] {message}")


# =========== Глобальный экземпляр ===========

# Ленивая инициализация: глобальный объект создаётся при первом обращении,
# а не при импорте, чтобы импорт модуля не падал, если DLL недоступна.

class _LazyAutomator:
    """Прокси, инициализирующий Automator при первом обращении к атрибуту."""

    _real = None

    def _get(self):
        if self.__class__._real is None:
            self.__class__._real = Automator()
        return self.__class__._real

    def __getattr__(self, name):
        return getattr(self._get(), name)

    def __setattr__(self, name, value):
        setattr(self._get(), name, value)


automator = _LazyAutomator()


# =========== Экспорт ===========

__all__ = [
    "Automator",
    "automator",
    "MouseAction",
    "MOUSEEVENTF_MOVE",
    "MOUSEEVENTF_LEFTDOWN",
    "MOUSEEVENTF_LEFTUP",
    "MOUSEEVENTF_RIGHTDOWN",
    "MOUSEEVENTF_RIGHTUP",
    "MOUSEEVENTF_MIDDLEDOWN",
    "MOUSEEVENTF_MIDDLEUP",
    "MOUSEEVENTF_ABSOLUTE",
    "VK_SHIFT",
    "VK_CONTROL",
    "VK_ALT",
    "VK_ENTER",
    "VK_TAB",
    "VK_ESCAPE",
    "VK_SPACE",
    "VK_BACK",
    "VK_DELETE",
    "VK_HOME",
    "VK_END",
    "VK_PAGEUP",
    "VK_PAGEDOWN",
    "VK_UP",
    "VK_DOWN",
    "VK_LEFT",
    "VK_RIGHT",
]


# =========== Тест при прямом запуске ===========

if __name__ == "__main__":
    logging.basicConfig(level=logging.DEBUG)
    print("Тестирование wrapper_automator.py")
    print("=" * 50)

    try:
        a = Automator()
        print(f"Библиотека загружена из: {a._loaded_path}")
        print(f"\nMOUSEEVENTF_LEFTDOWN = {MOUSEEVENTF_LEFTDOWN:#06x}")
        print(f"VK_ENTER = {VK_ENTER:#04x}")
    except Exception as e:
        print(f"Ошибка: {e}")
        sys.exit(1)
