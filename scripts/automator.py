import ctypes
from ctypes import c_int, c_char_p, c_void_p, Structure, POINTER, byref
import time

# Загружаем библиотеку libautomator.dll
try:
    # Путь к libautomator.dll
    #automator = ctypes.WinDLL("../lib/libautomator.dll")
    # Или можно указать полный путь:
    automator = ctypes.WinDLL("D:/Projects/automator/lib/libautomator.dll")
except OSError as e:
    print(f"Ошибка загрузки библиотеки: {e}")
    print("Убедитесь, что библиотека находится в текущей директории или в PATH")
    exit(1)

# Определяем структуру MouseAction аналогично C-структуре
class MouseAction(Structure):
    _fields_ = [
        ("x", c_int),      # Координата X
        ("y", c_int),      # Координата Y
        ("dwFlags", ctypes.c_ulong),  # Флаги действия (DWORD)
    ]

# Определяем константы для флагов мыши (обычные значения из WinAPI)
MOUSEEVENTF_MOVE = 0x0001
MOUSEEVENTF_LEFTDOWN = 0x0002
MOUSEEVENTF_LEFTUP = 0x0004
MOUSEEVENTF_RIGHTDOWN = 0x0008
MOUSEEVENTF_RIGHTUP = 0x0010
MOUSEEVENTF_MIDDLEDOWN = 0x0020
MOUSEEVENTF_MIDDLEUP = 0x0040
MOUSEEVENTF_ABSOLUTE = 0x8000

# Настраиваем типы аргументов и возвращаемых значений для функций
# simulate_keystroke(const char *text)
automator.simulate_keystroke.argtypes = [c_char_p]
automator.simulate_keystroke.restype = None

# simulate_mouse_sequence(const MouseAction *actions, int count)
automator.simulate_mouse_sequence.argtypes = [POINTER(MouseAction), c_int]
automator.simulate_mouse_sequence.restype = None

# simulate_mouse_click_at(int x, int y)
automator.simulate_mouse_click_at.argtypes = [c_int, c_int]
automator.simulate_mouse_click_at.restype = None

# Обертки для удобства использования
def simulate_keystroke(text):
    """
    Симуляция нажатия клавиш для строки
    """
    # Преобразуем строку в bytes (ASCII или UTF-8)
    if isinstance(text, str):
        text = text.encode('utf-8')
    automator.simulate_keystroke(text)

def simulate_mouse_sequence_python(actions_list):
    """
    Эмуляция последовательности действий мыши
    actions_list: список словарей или кортежей вида [{'x': 100, 'y': 200, 'flags': flags}, ...]
    """
    count = len(actions_list)
    # Создаем массив структур MouseAction
    actions_array = (MouseAction * count)()
    for i, action in enumerate(actions_list):
        if isinstance(action, dict):
            actions_array[i].x = action['x']
            actions_array[i].y = action['y']
            actions_array[i].dwFlags = action.get('flags', 0)
        else:
            # Если передали кортеж (x, y, flags)
            actions_array[i].x = action[0]
            actions_array[i].y = action[1]
            actions_array[i].dwFlags = action[2] if len(action) > 2 else 0
    automator.simulate_mouse_sequence(actions_array, count)

def simulate_mouse_click_at(x, y):
    """
    Клик мыши в указанных координатах
    """
    automator.simulate_mouse_click_at(x, y)

def simulate_mouse_click(x, y, button='left'):
    """
    Клик указанной кнопкой мыши в координатах
    button: 'left', 'right', 'middle'
    """
    # Создаем последовательность: нажатие и отпускание
    flags_down = MOUSEEVENTF_LEFTDOWN
    flags_up = MOUSEEVENTF_LEFTUP
    if button == 'right':
        flags_down = MOUSEEVENTF_RIGHTDOWN
        flags_up = MOUSEEVENTF_RIGHTUP
    elif button == 'middle':
        flags_down = MOUSEEVENTF_MIDDLEDOWN
        flags_up = MOUSEEVENTF_MIDDLEUP
    actions = [
        (x, y, flags_down),
        (x, y, flags_up)
    ]
    simulate_mouse_sequence_python(actions)

def simulate_mouse_drag(start_x, start_y, end_x, end_y, button='left'):
    """
    Перетаскивание мышью из одной точки в другую
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
        (end_x, end_y, 0),              # Перемещение с зажатой кнопкой
        (end_x, end_y, flags_up)        # Отпускание кнопки
    ]
    simulate_mouse_sequence_python(actions)

def simulate_mouse_move(x, y):
    """
    Перемещение мыши в указанные координаты
    """
    actions = [(x, y, MOUSEEVENTF_MOVE)]
    simulate_mouse_sequence_python(actions)

# Примеры использования
if __name__ == "__main__":
    print("Тестирование библиотеки automator...")
    
    # Пример 1: Ввод текста
    print("Ввод текста 'Hello World!'")
    time.sleep(2)  # Пауза 2 секунды, чтобы переключиться в нужное окно
    simulate_keystroke("Hello World!")
    
    # Пример 2: Клик левой кнопкой мыши
    print("Клик мыши в точке (500, 300)")
    time.sleep(1)
    simulate_mouse_click_at(500, 300)
    
    # Пример 3: Использование simulate_mouse_click
    print("Клик правой кнопкой в точке (600, 400)")
    time.sleep(1)
    simulate_mouse_click(600, 400, button='right')
    
    # Пример 4: Перетаскивание
    print("Перетаскивание из (100, 100) в (200, 200)")
    time.sleep(1)
    simulate_mouse_drag(100, 100, 200, 200)
    
    # Пример 5: Рисование прямоугольника мышью
    print("Рисование прямоугольника")
    time.sleep(1)
    
    # Координаты прямоугольника
    x, y = 300, 300
    width, height = 100, 100
    
    rectangle_actions = [
        (x, y, MOUSEEVENTF_LEFTDOWN),              # Нажатие в начальной точке
        (x + width, y, 0),                         # Перемещение вправо
        (x + width, y + height, 0),                # Перемещение вниз
        (x, y + height, 0),                        # Перемещение влево
        (x, y, 0),                                 # Возврат в начало
        (x, y, MOUSEEVENTF_LEFTUP)                 # Отпускание кнопки
    ]
    
    simulate_mouse_sequence_python(rectangle_actions)
    
    print("Тестирование завершено!")