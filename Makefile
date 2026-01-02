# Главный Makefile - Управление всем проектом

# Подпроекты
LIB_DIR = lib
GUI_QT_DIR = gui-qt
CONSOLE_DIR = console

# Утилиты
MKDIR = mkdir -p
RM = del /Q 2>nul || true

# Правила по умолчанию
all: lib console

# Сборка библиотеки
lib:
	cd $(LIB_DIR) && $(MAKE) static

# Сборка Qt GUI (требует установленного Qt)
qt: lib
	@echo "Сборка Qt приложения..."
	cd $(GUI_QT_DIR) && qmake Automator.pro
	cd $(GUI_QT_DIR) && $(MAKE)
	@echo "Qt приложение собрано: $(GUI_QT_DIR)/automator_qt.exe"

# Сборка консольной версии
console: lib
	cd $(CONSOLE_DIR) && $(MAKE)

# Сборка всего (библиотека + консольная версия)
everything: lib console
	@echo "Все компоненты собраны:"
	@echo "  - Библиотека: $(LIB_DIR)/libautomator.a"
	@echo "  - Консольное приложение: $(CONSOLE_DIR)/automator_console.exe"

# Очистка всего проекта
clean:
	cd $(LIB_DIR) && $(MAKE) clean
	cd $(GUI_QT_DIR) && $(MAKE) clean 2>nul || true
	cd $(CONSOLE_DIR) && $(MAKE) clean
	$(RM) *.exe
	$(RM) gui-qt/Makefile
	$(RM) gui-qt/release gui-qt/debug

# Запуск консольной версии
run-console: console
	cd $(CONSOLE_DIR) && $(MAKE) run

# Запуск Qt версии
run-qt: qt
	cd $(GUI_QT_DIR) && ./automator_qt.exe

# Создание дистрибутива
dist: clean everything
	@echo "Создание дистрибутива..."
	$(MKDIR) dist
	$(MKDIR) dist/include
	$(MKDIR) dist/lib
	$(MKDIR) dist/bin
	$(MKDIR) dist/examples
	copy $(LIB_DIR)\automator.h dist\include\ 2>nul || true
	copy $(LIB_DIR)\libautomator.a dist\lib\ 2>nul || true
	copy $(CONSOLE_DIR)\*.exe dist\bin\ 2>nul || true
	copy README.md dist\ 2>nul || true
	@echo "Дистрибутив создан в папке 'dist'"

# Проверка зависимостей
check-deps:
	@echo "Проверка зависимостей..."
	@where gcc >nul 2>nul && echo "✓ GCC установлен" || echo "✗ GCC не найден"
	@where qmake >nul 2>nul && echo "✓ qmake установлен" || echo "✗ qmake не найден (нужен для Qt)"
	@where make >nul 2>nul && echo "✓ make установлен" || echo "✗ make не найден"

# Помощь
help:
	@echo "Доступные команды:"
	@echo "  make all         - Собрать библиотеку и консольную версию (по умолчанию)"
	@echo "  make lib         - Только библиотеку"
	@echo "  make qt          - Qt GUI приложение (требует qmake)"
	@echo "  make console     - Консольное приложение"
	@echo "  make everything  - Всё (библиотека + консольная версия)"
	@echo "  make clean       - Очистить все собранные файлы"
	@echo "  make run-console - Запустить консольную версию"
	@echo "  make run-qt      - Собрать и запустить Qt версию"
	@echo "  make dist        - Создать дистрибутив"
	@echo "  make check-deps  - Проверить установленные зависимости"
	@echo ""
	@echo "Примечание: для сборки Qt версии требуется установленный Qt и qmake"

.PHONY: all lib qt console everything clean run-console run-qt dist check-deps help