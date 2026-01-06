# lib/Makefile - Сборка библиотеки libautomator

CC = gcc
AR = ar
CFLAGS = -O2 -Wall -Wextra -I. -fPIC
CFLAGS += -DBUILDING_AUTOMATOR_DLL
LDFLAGS_DLL = -shared -Wl,--out-implib,libautomator.dll.a

# Используем pkg-config для OpenCV
PKG_CONFIG = pkg-config
OPENCV_CFLAGS := $(shell $(PKG_CONFIG) --cflags opencv4 2>/dev/null || echo "")
OPENCV_LIBS := $(shell $(PKG_CONFIG) --libs opencv4 2>/dev/null || echo "")

# Если pkg-config не сработал, используем стандартные пути MSYS2
ifeq ($(OPENCV_CFLAGS),)
    OPENCV_CFLAGS = -I/mingw64/include/opencv4 -DHAVE_OPENCV
endif

ifeq ($(OPENCV_LIBS),)
    OPENCV_LIBS = -L/mingw64/lib \
                  -lopencv_core \
                  -lopencv_imgproc \
                  -lopencv_highgui \
                  -lopencv_imgcodecs
endif

# Добавляем флаги OpenCV
CFLAGS += $(OPENCV_CFLAGS)

# Имена файлов
LIB_NAME = libautomator
STATIC_LIB = $(LIB_NAME).a
DYNAMIC_LIB = $(LIB_NAME).dll
DLL_IMPLIB = $(LIB_NAME).dll.a

# Исходные файлы
SOURCES = keyboard.c mouse.c screen.c ocr.c opencv.c window.c
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
	$(CC) $(LDFLAGS_DLL) -o $@ $(OBJECTS) -lgdi32 -luser32 $(OPENCV_LIBS)

# Компиляция исходников
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Очистка с использованием MinGW команд
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

.PHONY: all static dynamic clean
