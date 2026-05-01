# Главный Makefile - Управление всем проектом
#
# Окружение: MSYS2 (mingw64). Утилиты POSIX (rm, mkdir -p, cp).
# Версия проекта (передаётся в C через AUTOMATOR_VERSION).

AUTOMATOR_VERSION ?= 0.2.0

LIB_DIR = lib
GUI_QT_DIR = gui-qt
CONSOLE_DIR = console

# Каталог установки (можно переопределить: `make install PREFIX=...`)
PREFIX ?= /usr/local
DESTDIR ?=

export AUTOMATOR_VERSION

all: lib console

lib:
	$(MAKE) -C $(LIB_DIR) static dynamic

qt: lib
	@echo "Building Qt application..."
	cd $(GUI_QT_DIR) && qmake Automator.pro
	$(MAKE) -C $(GUI_QT_DIR)
	@echo "Qt application built: $(GUI_QT_DIR)/automator_qt.exe"

console: lib
	$(MAKE) -C $(CONSOLE_DIR)

everything: lib console
	@echo "All components built:"
	@echo "  - Library:           $(LIB_DIR)/libautomator.a"
	@echo "  - Library (DLL):     $(LIB_DIR)/libautomator.dll"
	@echo "  - Console app:       $(CONSOLE_DIR)/automator_console.exe"

clean:
	$(MAKE) -C $(LIB_DIR) clean
	$(MAKE) -C $(CONSOLE_DIR) clean
	-$(MAKE) -C $(GUI_QT_DIR) clean 2>/dev/null || true
	@rm -rf dist
	@rm -f $(GUI_QT_DIR)/Makefile
	@rm -rf $(GUI_QT_DIR)/release $(GUI_QT_DIR)/debug

run-console: console
	$(MAKE) -C $(CONSOLE_DIR) run

run-qt: qt
	cd $(GUI_QT_DIR) && ./automator_qt.exe

# Создание дистрибутива в ./dist
dist: clean everything
	@echo "Creating distribution..."
	@mkdir -p dist/include dist/lib dist/bin dist/scripts
	cp $(LIB_DIR)/automator.h $(LIB_DIR)/keyboard.h $(LIB_DIR)/mouse.h \
	   $(LIB_DIR)/screen.h $(LIB_DIR)/opencv.h $(LIB_DIR)/ocr.h \
	   $(LIB_DIR)/window.h $(LIB_DIR)/log.h dist/include/
	cp $(LIB_DIR)/libautomator.a dist/lib/ 2>/dev/null || true
	cp $(LIB_DIR)/libautomator.dll dist/bin/ 2>/dev/null || true
	cp $(CONSOLE_DIR)/automator_console.exe dist/bin/ 2>/dev/null || true
	cp scripts/wrapper_automator.py dist/scripts/
	cp README.md LICENSE dist/ 2>/dev/null || true
	@echo "Distribution created in ./dist (version $(AUTOMATOR_VERSION))"

# Установка в $(DESTDIR)$(PREFIX)
install: everything
	@echo "Installing to $(DESTDIR)$(PREFIX)..."
	@mkdir -p $(DESTDIR)$(PREFIX)/bin
	@mkdir -p $(DESTDIR)$(PREFIX)/lib
	@mkdir -p $(DESTDIR)$(PREFIX)/include/automator
	cp $(LIB_DIR)/libautomator.a $(DESTDIR)$(PREFIX)/lib/
	cp $(LIB_DIR)/libautomator.dll $(DESTDIR)$(PREFIX)/bin/
	cp $(LIB_DIR)/automator.h $(LIB_DIR)/keyboard.h $(LIB_DIR)/mouse.h \
	   $(LIB_DIR)/screen.h $(LIB_DIR)/opencv.h $(LIB_DIR)/ocr.h \
	   $(LIB_DIR)/window.h $(LIB_DIR)/log.h $(DESTDIR)$(PREFIX)/include/automator/
	cp $(CONSOLE_DIR)/automator_console.exe $(DESTDIR)$(PREFIX)/bin/
	@echo "Installation complete."

uninstall:
	@echo "Uninstalling from $(DESTDIR)$(PREFIX)..."
	@rm -f $(DESTDIR)$(PREFIX)/lib/libautomator.a
	@rm -f $(DESTDIR)$(PREFIX)/bin/libautomator.dll
	@rm -f $(DESTDIR)$(PREFIX)/bin/automator_console.exe
	@rm -rf $(DESTDIR)$(PREFIX)/include/automator

# Версия и зависимости
version:
	@echo "Automator $(AUTOMATOR_VERSION)"

check-deps:
	@echo "Checking dependencies..."
	@command -v gcc >/dev/null 2>&1 && echo "[ok] gcc" || echo "[missing] gcc"
	@command -v g++ >/dev/null 2>&1 && echo "[ok] g++" || echo "[missing] g++"
	@command -v make >/dev/null 2>&1 && echo "[ok] make" || echo "[missing] make"
	@command -v qmake >/dev/null 2>&1 && echo "[ok] qmake (for Qt build)" || echo "[skip] qmake (Qt build will be unavailable)"
	@pkg-config --exists opencv4 2>/dev/null && echo "[ok] OpenCV 4" || echo "[skip] OpenCV (CV functions disabled)"
	@command -v python >/dev/null 2>&1 && echo "[ok] python" || echo "[missing] python"

# Python-тесты
test-python:
	cd scripts && python -m pytest -v

help:
	@echo "Targets:"
	@echo "  make all         - Build library and console app (default)"
	@echo "  make lib         - Build only the library"
	@echo "  make qt          - Build Qt GUI (requires qmake)"
	@echo "  make console     - Build console app"
	@echo "  make everything  - Build everything"
	@echo "  make clean       - Remove all build artifacts"
	@echo "  make run-console - Build & run console app"
	@echo "  make run-qt      - Build & run Qt GUI"
	@echo "  make dist        - Create distribution in ./dist"
	@echo "  make install     - Install to \$$DESTDIR\$$PREFIX (default /usr/local)"
	@echo "  make uninstall   - Remove installed files"
	@echo "  make version     - Print project version"
	@echo "  make check-deps  - Check installed tools"
	@echo "  make test-python - Run Python tests (pytest)"

.PHONY: all lib qt console everything clean run-console run-qt dist install \
        uninstall version check-deps help test-python
