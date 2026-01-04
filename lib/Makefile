# lib/Makefile - Сборка библиотеки libautomator

CC = gcc
AR = ar
CFLAGS = -O2 -Wall -Wextra -I. -fPIC
LDFLAGS_DLL = -shared -Wl,--subsystem,windows,--out-implib,libautomator.dll.a

# Имена файлов
LIB_NAME = libautomator
STATIC_LIB = $(LIB_NAME).a
DYNAMIC_LIB = $(LIB_NAME).dll
DLL_IMPLIB = $(LIB_NAME).dll.a

# Исходные файлы
SOURCES = keyboard.c mouse.c screen.c ocr.c
OBJECTS = $(SOURCES:.c=.o)
HEADERS = automator.h

# По умолчанию собираем обе библиотеки
all: static dynamic

# Статическая библиотека
static: $(STATIC_LIB)

# Динамическая библиотека
dynamic: $(DYNAMIC_LIB)

$(STATIC_LIB): $(OBJECTS)
	$(AR) rcs $@ $^

$(DYNAMIC_LIB): $(OBJECTS)
	$(CC) $(LDFLAGS_DLL) -o $@ $(OBJECTS) -lgdi32 -luser32

# Компиляция исходников
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Очистка с использованием MinGW команд (как в console/Makefile)
clean:
	@echo "Очистка библиотеки..."
	@if [ -f "$(STATIC_LIB)" ]; then rm -f "$(STATIC_LIB)" && echo "  Удален: $(STATIC_LIB)"; else echo "  $(STATIC_LIB) не найден"; fi
	@if [ -f "$(DYNAMIC_LIB)" ]; then rm -f "$(DYNAMIC_LIB)" && echo "  Удален: $(DYNAMIC_LIB)"; else echo "  $(DYNAMIC_LIB) не найден"; fi
	@if [ -f "$(DLL_IMPLIB)" ]; then rm -f "$(DLL_IMPLIB)" && echo "  Удален: $(DLL_IMPLIB)"; else echo "  $(DLL_IMPLIB) не найден"; fi
	@for file in *.o; do \
		if [ -f "$$file" ]; then \
			rm -f "$$file" && echo "  Удален: $$file"; \
		fi \
	done
	@if [ -f "nul" ]; then rm -f "nul" && echo "  Удален: nul"; else echo "  Файл nul не найден"; fi
	@if [ -f "NUL" ]; then rm -f "NUL" && echo "  Удален: NUL"; else echo "  Файл NUL не найден"; fi
	@echo "Очистка завершена"

# Альтернативная очистка через cmd
clean2:
	@echo "Очистка библиотеки (через cmd)..."
	@cmd //c "if exist $(STATIC_LIB) (del /Q $(STATIC_LIB) >nul 2>&1 && echo   Удален: $(STATIC_LIB)) else (echo   $(STATIC_LIB) не найден)"
	@cmd //c "if exist $(DYNAMIC_LIB) (del /Q $(DYNAMIC_LIB) >nul 2>&1 && echo   Удален: $(DYNAMIC_LIB)) else (echo   $(DYNAMIC_LIB) не найден)"
	@cmd //c "if exist $(DLL_IMPLIB) (del /Q $(DLL_IMPLIB) >nul 2>&1 && echo   Удален: $(DLL_IMPLIB)) else (echo   $(DLL_IMPLIB) не найден)"
	@cmd //c "for %%f in (*.o) do (if exist %%f (del /Q %%f >nul 2>&1 && echo   Удален: %%f))"
	@cmd //c "if exist nul (del /Q nul >nul 2>&1 && echo   Удален: nul) else (echo   Файл nul не найден)"
	@cmd //c "if exist NUL (del /Q NUL >nul 2>&1 && echo   Удален: NUL) else (echo   Файл NUL не найден)"
	@echo "Очистка завершена"

# Простая очистка
clean3:
	@echo "Очистка библиотеки..."
	-@rm -f $(STATIC_LIB) $(DYNAMIC_LIB) $(DLL_IMPLIB) 2>/dev/null || true
	-@rm -f *.o 2>/dev/null || true
	-@rm -f nul 2>/dev/null || true
	-@rm -f NUL 2>/dev/null || true
	@echo "Очистка завершена"

# Установка с использованием MinGW команд
install: all
	@echo "Установка библиотеки..."
	@mkdir -p ../../include 2>/dev/null || true
	@mkdir -p ../../lib 2>/dev/null || true
	@if [ -f "automator.h" ]; then cp automator.h ../../include/ && echo "  automator.h -> ../../include/"; else echo "  automator.h не найден"; fi
	@if [ -f "$(STATIC_LIB)" ]; then cp $(STATIC_LIB) ../../lib/ && echo "  $(STATIC_LIB) -> ../../lib/"; else echo "  $(STATIC_LIB) не найден"; fi
	@if [ -f "$(DYNAMIC_LIB)" ]; then cp $(DYNAMIC_LIB) ../../lib/ && echo "  $(DYNAMIC_LIB) -> ../../lib/"; else echo "  $(DYNAMIC_LIB) не найден"; fi
	@if [ -f "$(DLL_IMPLIB)" ]; then cp $(DLL_IMPLIB) ../../lib/ && echo "  $(DLL_IMPLIB) -> ../../lib/"; else echo "  $(DLL_IMPLIB) не найден"; fi
	@echo "Установка завершена"

# Альтернативная установка через cmd
install2: all
	@echo "Установка библиотеки (через cmd)..."
	@cmd //c "if not exist ..\..\include mkdir ..\..\include >nul 2>&1"
	@cmd //c "if not exist ..\..\lib mkdir ..\..\lib >nul 2>&1"
	@cmd //c "if exist automator.h (copy automator.h ..\..\include\ >nul 2>&1 && echo   automator.h -> ..\..\include\) else (echo   automator.h не найден)"
	@cmd //c "if exist $(STATIC_LIB) (copy $(STATIC_LIB) ..\..\lib\ >nul 2>&1 && echo   $(STATIC_LIB) -> ..\..\lib\) else (echo   $(STATIC_LIB) не найден)"
	@cmd //c "if exist $(DYNAMIC_LIB) (copy $(DYNAMIC_LIB) ..\..\lib\ >nul 2>&1 && echo   $(DYNAMIC_LIB) -> ..\..\lib\) else (echo   $(DYNAMIC_LIB) не найден)"
	@cmd //c "if exist $(DLL_IMPLIB) (copy $(DLL_IMPLIB) ..\..\lib\ >nul 2>&1 && echo   $(DLL_IMPLIB) -> ..\..\lib\) else (echo   $(DLL_IMPLIB) не найден)"
	@echo "Установка завершена"

# Показать содержимое директории
list:
	@echo "Содержимое директории:"
	@ls -la 2>/dev/null || dir

.PHONY: all static dynamic clean clean2 clean3 install install2 list