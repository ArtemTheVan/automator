"""
user_script.py - Пример пользовательского скрипта автоматизации
Показывает как использовать глобальный экземпляр automator в реальных задачах.
"""

import time
from wrapper_automator import automator

class UserAutomationTask:
    """Класс для организации сложных задач автоматизации."""
    
    def __init__(self, task_name):
        self.task_name = task_name
        self.step_count = 0
    
    def log_step(self, description):
        """Логирование шага выполнения."""
        self.step_count += 1
        automator.info(f"[{self.task_name}] Шаг {self.step_count}: {description}")
    
    def execute(self):
        """Основной метод выполнения задачи."""
        automator.info(f"Начинаю выполнение задачи: {self.task_name}")
        
        try:
            self._prepare()
            self._run_steps()
            self._complete()
            
            automator.info(f"Задача '{self.task_name}' выполнена успешно!")
            automator.info(f"Всего шагов: {self.step_count}")
            
        except Exception as e:
            automator.error(f"Ошибка при выполнении задачи: {e}")
            raise
    
    def _prepare(self):
        """Подготовительные действия."""
        self.log_step("Подготовка к выполнению")
        automator.sleep(2)  # Даем время пользователю переключиться
    
    def _run_steps(self):
        """Основные шаги выполнения (переопределяется в наследниках)."""
        pass
    
    def _complete(self):
        """Завершающие действия."""
        self.log_step("Завершение задачи")
        automator.sleep(1)

class NotepadAutomation(UserAutomationTask):
    """Автоматизация работы с Блокнотом."""
    
    def __init__(self):
        super().__init__("Автоматизация Блокнота")
    
    def _run_steps(self):
        """Шаги автоматизации Блокнота."""
        
        # Шаг 1: Открыть Блокнот
        self.log_step("Открытие Блокнота")
        automator.keystroke("notepad")
        automator.sleep(0.5)
        automator.keystroke("\n")  # Enter
        automator.sleep(2)
        
        # Шаг 2: Ввести заголовок
        self.log_step("Ввод заголовка")
        automator.keystroke("=" * 50)
        automator.keystroke("\n")
        automator.keystroke("АВТОМАТИЧЕСКИ СОЗДАННЫЙ ДОКУМЕНТ")
        automator.keystroke("\n")
        automator.keystroke("=" * 50)
        automator.keystroke("\n\n")
        
        # Шаг 3: Ввести основной текст
        self.log_step("Ввод основного текста")
        lines = [
            "Этот документ был создан автоматически",
            "с использованием библиотеки automator.",
            "",
            "Дата создания: " + time.strftime("%d.%m.%Y %H:%M:%S"),
            "Автор: Автоматизированная система",
        ]
        
        for line in lines:
            automator.keystroke(line)
            automator.keystroke("\n")
        
        # Шаг 4: Сохранить документ
        self.log_step("Сохранение документа")
        automator.sleep(1)
        automator.keystroke("\x13")  # Ctrl+S
        automator.sleep(1)
        
        # Ввод имени файла
        filename = f"автоматический_документ_{time.strftime('%Y%m%d_%H%M%S')}.txt"
        automator.keystroke(filename)
        automator.sleep(0.5)
        automator.keystroke("\n")  # Enter
        
        # Шаг 5: Закрыть Блокнот
        self.log_step("Закрытие Блокнота")
        automator.sleep(1)
        automator.keystroke("\x00\xF4")  # Alt+F4

class MouseDrawingTask(UserAutomationTask):
    """Задача рисования мышью."""
    
    def __init__(self):
        super().__init__("Рисование мышью")
    
    def _run_steps(self):
        """Шаги рисования."""
        
        # Рисуем квадрат
        self.log_step("Рисование квадрата")
        automator.rectangle(400, 300, 200, 200)
        automator.sleep(1)
        
        # Рисуем треугольник
        self.log_step("Рисование треугольника")
        actions = [
            (600, 400, MOUSEEVENTF_LEFTDOWN),
            (700, 500, 0),
            (500, 500, 0),
            (600, 400, 0),
            (600, 400, MOUSEEVENTF_LEFTUP),
        ]
        automator.mouse_sequence(actions)
        automator.sleep(1)
        
        # Рисуем круг (аппроксимация)
        self.log_step("Рисование круга")
        self._draw_circle(600, 400, 100)
    
    def _draw_circle(self, center_x, center_y, radius):
        """Рисование круга (аппроксимация многоугольником)."""
        import math
        
        points = []
        for i in range(0, 361, 10):
            angle = math.radians(i)
            x = center_x + radius * math.cos(angle)
            y = center_y + radius * math.sin(angle)
            points.append((int(x), int(y)))
        
        # Начинаем рисование
        automator.mouse_sequence([(points[0][0], points[0][1], MOUSEEVENTF_LEFTDOWN)])
        
        # Рисуем сегменты
        for x, y in points[1:]:
            automator.mouse_sequence([(x, y, 0)])
        
        # Завершаем
        automator.mouse_sequence([(points[0][0], points[0][1], 0)])
        automator.mouse_sequence([(points[0][0], points[0][1], MOUSEEVENTF_LEFTUP)])

def main():
    """Основная функция пользовательского скрипта."""
    
    automator.info("=" * 60)
    automator.info("ЗАПУСК ПОЛЬЗОВАТЕЛЬСКОГО СКРИПТА АВТОМАТИЗАЦИИ")
    automator.info("=" * 60)
    
    # Создаем и выполняем задачи
    tasks = [
        NotepadAutomation(),
        MouseDrawingTask(),
    ]
    
    for task in tasks:
        try:
            task.execute()
            automator.info(f"\nЗадача '{task.task_name}' завершена успешно!\n")
            automator.sleep(2)  # Пауза между задачами
        except Exception as e:
            automator.error(f"Задача '{task.task_name}' завершилась с ошибкой: {e}")
            break
    
    automator.info("=" * 60)
    automator.info("ВСЕ ЗАДАЧИ ВЫПОЛНЕНЫ")
    automator.info("=" * 60)

if __name__ == "__main__":
    # Импортируем здесь, чтобы избежать циклического импорта
    from wrapper_automator import MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP
    main()