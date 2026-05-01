# Automator — описание проекта

## Содержание

- [1. Назначение](#1-назначение)
- [2. Архитектура](#2-архитектура)
- [3. Публичный API библиотеки](#3-публичный-api-библиотеки)
  - [3.1. Клавиатура](#31-клавиатура)
  - [3.2. Мышь](#32-мышь)
  - [3.3. Экран](#33-экран)
  - [3.4. Компьютерное зрение (OpenCV)](#34-компьютерное-зрение-opencv)
  - [3.5. OCR (Tesseract)](#35-ocr-tesseract)
- [4. Python-обёртка](#4-python-обёртка)
- [5. Сборка](#5-сборка)
  - [5.1. Зависимости](#51-зависимости)
  - [5.2. Команды корневого Makefile](#52-команды-корневого-makefile)
- [6. Использование](#6-использование)
  - [6.1. Консольное приложение](#61-консольное-приложение)
  - [6.2. Qt-редактор сценариев](#62-qt-редактор-сценариев)
  - [6.3. Python-скрипт напрямую](#63-python-скрипт-напрямую)
- [7. Структура репозитория](#7-структура-репозитория)
- [8. Платформа и ограничения](#8-платформа-и-ограничения)

## 1. Назначение

**Automator** — кроссязыковой инструмент автоматизации действий пользователя в среде Windows.
Он объединяет нативную C/C++ библиотеку `libautomator`, консольное приложение, Qt-редактор сценариев
и Python-обёртку, позволяющую писать скрипты автоматизации на высокоуровневом языке.

Ключевые возможности:

- Эмуляция ввода с клавиатуры через `SendInput` с `KEYEVENTF_UNICODE` — поддержка
  любых символов UTF-8 / Unicode (включая кириллицу и эмодзи) независимо от
  текущей раскладки.
- Эмуляция действий мыши с поддержкой нескольких мониторов через виртуальный
  экран (`MOUSEEVENTF_VIRTUALDESK`).
- Per-monitor DPI awareness через `automator_init()`.
- Захват экрана и его регионов в BMP-файлы и `HBITMAP`.
- Поиск визуальных объектов с помощью OpenCV (детекция системного трея, текстовых регионов,
  индикатора раскладки клавиатуры).
- Распознавание текста (OCR) на базе Tesseract — английский, русский и совместный режимы.
- Определение текущей раскладки клавиатуры активного окна (WinAPI `GetKeyboardLayout`).
- Графический редактор Python-сценариев на Qt с подсветкой, шаблонами и запуском подпроцессов.

## 2. Архитектура

Проект разделён на четыре независимых, но связанных подпроекта:

| Подпроект | Содержимое и роль |
|-----------|-------------------|
| `lib/`     | Ядро системы — статическая (`libautomator.a`) и динамическая (`libautomator.dll`) библиотека. Исходники на C (`keyboard.c`, `mouse.c`, `screen.c`, `ocr.c`, `window.c`) и C++ обёртка над OpenCV (`opencv_wrapper.cpp`). Публичный API — `automator.h`. |
| `console/` | Консольное приложение `automator_console.exe`. Демонстрирует возможности библиотеки: определение раскладки, тестовая детекция OpenCV, полное распознавание текста системного трея. |
| `gui-qt/`  | Графический редактор сценариев `automator_qt.exe` на Qt 5/6. Включает редактор кода с нумерацией строк, шаблоны, запись/воспроизведение, диалог настроек путей (Python, MinGW, OpenCV, библиотека). |
| `scripts/` | Python-обёртка `wrapper_automator.py` и набор демо-скриптов (`simple_demo.py`, `qt_example_script.py`, `test_*.py`). Скрипты загружают `automator.dll` через `ctypes`. |

## 3. Публичный API библиотеки

Все функции экспортируются с атрибутом `AUTOMATOR_API` (`__declspec(dllexport)` /
`dllimport`) и доступны из C, C++ и Python.

### 3.1. Клавиатура

```c
void simulate_keystroke(const char *text);
```

Отправляет в активное окно строку любых символов, включая кириллицу. Внутри использует
`SendInput` со сценариями Unicode и нативных VK-кодов.

### 3.2. Мышь

```c
typedef struct {
    int   x;
    int   y;
    DWORD dwFlags;   // MOUSEEVENTF_*
} MouseAction;

void simulate_mouse_sequence(const MouseAction *actions, int count);
void simulate_mouse_click_at(int x, int y);
```

Поддерживает абсолютное и относительное перемещение, отдельные события нажатия/отпускания
левой/правой/средней кнопок (`MOUSEEVENTF_LEFTDOWN`, `MOUSEEVENTF_RIGHTUP`, …).

### 3.3. Экран

```c
typedef struct { int x, y, width, height; } ScreenRegion;

int          capture_screen_region(int x, int y, int w, int h, const char *file);
HBITMAP      capture_to_bitmap(int x, int y, int w, int h);
int          save_bitmap_to_file(HBITMAP bmp, const char *file);
int          get_screen_width(void);
int          get_screen_height(void);
ScreenRegion get_system_tray_region(void);
ScreenRegion get_system_tray_region_auto(void);
```

### 3.4. Компьютерное зрение (OpenCV)

```c
typedef struct {
    int   x, y, width, height;
    float confidence;
} DetectedRegion;

int             opencv_init(void);
void            opencv_cleanup(void);
DetectedRegion  detect_system_tray_region_opencv(void);
DetectedRegion *find_text_regions_in_tray(ScreenRegion region, int *region_count);
DetectedRegion  find_keyboard_layout_in_region(ScreenRegion region);
```

Сборка опционально подключает OpenCV 4 через `pkg-config`. Если OpenCV не найден, библиотека
собирается с флагом `NO_OPENCV` и использует упрощённую детекцию.

### 3.5. OCR (Tesseract)

```c
typedef struct {
    char  *text;
    float  confidence;
    int    text_length;
    int    error_code;     // OCR_SUCCESS, OCR_ERROR_*
} OCRResult;

int        ocr_init(const char *tessdata_path);
void       ocr_cleanup(void);
OCRResult  ocr_from_file(const char *image, const char *lang,
                         unsigned int psm, unsigned int dpi);
void       ocr_result_free(OCRResult *result);
```

Поддерживаемые языки: `eng`, `rus`, `eng+rus`. Параметры `psm` (page segmentation mode) и
`dpi` пробрасываются в Tesseract как есть.

## 4. Python-обёртка

Файл [scripts/wrapper_automator.py](scripts/wrapper_automator.py) реализует класс `Automator` (синглтон), который
загружает `automator.dll` через `ctypes` и предоставляет высокоуровневые методы:

```python
from wrapper_automator import automator

automator.keystroke("Привет от Python скрипта!")
automator.click(500, 300)
automator.move(600, 400)
automator.click(700, 350, button='right')
automator.rectangle(400, 200, 150, 150)
automator.drag(200, 200, 300, 300)
automator.sleep(1)
automator.info("Готово")
```

Константы WinAPI (`MOUSEEVENTF_*`, `VK_*`) переэкспортированы как атрибуты модуля.

## 5. Сборка

### 5.1. Зависимости

- GCC / MinGW-w64 (рекомендуется MSYS2 `mingw64`).
- GNU Make.
- Qt 5 или 6 + `qmake` — только для сборки GUI-редактора.
- OpenCV 4 (`pkg-config opencv4`) — опционально, для функций компьютерного зрения.
- Tesseract OCR + `tessdata` — опционально, для функций OCR.
- CPython 3.x — для запуска Python-сценариев.

Проверить установленные зависимости:

```powershell
make check-deps
```

### 5.2. Команды корневого Makefile

| Цель             | Действие |
|------------------|----------|
| `make all`         | Библиотека + консольное приложение (по умолчанию). |
| `make lib`         | Только статическая библиотека `lib/libautomator.a`. |
| `make qt`          | GUI-редактор `gui-qt/automator_qt.exe` (требует `qmake`). |
| `make console`     | Консольное приложение `console/automator_console.exe`. |
| `make everything`  | Библиотека и консольное приложение целиком. |
| `make run-console` | Сборка и запуск консольной версии. |
| `make run-qt`      | Сборка и запуск Qt-версии. |
| `make dist`        | Создание дистрибутива в каталоге `dist/`. |
| `make clean`       | Полная очистка артефактов сборки. |
| `make check-deps`  | Проверка установленного инструментария. |

## 6. Использование

### 6.1. Консольное приложение

```powershell
.\automator_console.exe            # Распознать весь текст системного трея
.\automator_console.exe -layout    # Только определить раскладку клавиатуры
.\automator_console.exe -test      # Тест детекции текста через OpenCV
.\automator_console.exe -h         # Справка
```

Артефакты отладки (BMP-снимки регионов) сохраняются рядом с исполняемым файлом с префиксом
`debug_` или `opencv_test_`.

### 6.2. Qt-редактор сценариев

`automator_qt.exe` запускает редактор Python-сценариев. Поддерживаются:

- Открытие/сохранение скриптов, список последних файлов.
- Шаблоны (через `QComboBox m_templateCombo`).
- Запуск активного скрипта в `QProcess` с потоковым выводом в `QTextBrowser`.
- Диалог настроек путей: интерпретатор Python, MinGW, OpenCV, `automator.dll`, каталог
  скриптов. Настройки сохраняются через `QSettings`.

При запуске редактор копирует в рабочий каталог `wrapper_automator.py`, `automator.dll` и
зависимости MinGW, чтобы скрипт работал автономно.

### 6.3. Python-скрипт напрямую

```powershell
$env:PATH = "C:\msys64\mingw64\bin;$env:PATH"
python scripts\simple_demo.py
```

Перед запуском убедитесь, что `automator.dll` и зависимости (`libgcc_s_seh-1.dll`,
`libstdc++-6.dll` и т. д.) находятся в `PATH` или рядом со скриптом.

## 7. Структура репозитория

```
automator/
├── Makefile                    # Корневой Makefile
├── README.md
├── lib/                        # Ядро — libautomator
│   ├── automator.h             # Публичный API
│   ├── keyboard.{c,h}
│   ├── mouse.{c,h}
│   ├── screen.{c,h}
│   ├── ocr.{c,h}
│   ├── opencv.h
│   ├── opencv_wrapper.cpp
│   ├── window.{c,h}
│   └── Makefile
├── console/                    # automator_console.exe
│   ├── main.c
│   └── Makefile
├── gui-qt/                     # automator_qt.exe (Qt)
│   ├── Automator.pro
│   ├── main.cpp
│   ├── automatorwidget.{cpp,h}
│   ├── settingsdialog.{cpp,h}
│   ├── automator_qt_resource.rc
│   └── icons/                  # load.png, play.png, record.png, save.png, stop.png
├── scripts/                    # Python-обёртка и демо
│   ├── wrapper_automator.py
│   ├── simple_demo.py
│   ├── qt_example_script.py
│   ├── user_script.py
│   ├── test_automator.py
│   └── test_python_script.py
└── docs/                       # Документация
    └── project.adoc
```

## 8. Платформа и ограничения

- Целевая ОС — **Windows 10** и новее (`_WIN32_WINNT=0x0A00`).
- Сборка ориентирована на MinGW-w64; MSVC не тестировался.
- Функции OpenCV/OCR опциональны; при их отсутствии библиотека собирается без них и
  предоставляет только эмуляцию ввода и захват экрана.
- GUI собирается со статической линковкой `libgcc`/`libstdc++` для упрощения дистрибуции.
