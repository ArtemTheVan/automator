# lib/Makefile - Сборка библиотеки libautomator

CC = gcc
AR = ar
CFLAGS = -O2 -Wall -Wextra -I. -fPIC
LDFLAGS_DLL = -shared -Wl,--subsystem,windows,--out-implib,libautomator.dll.a

# Имена файлов
LIB_NAME = libautomator
STATIC_LIB = $(LIB_NAME).a
DYNAMIC_LIB = $(LIB_NAME).dll

# Исходные файлы
SOURCES = keyboard.c mouse.c screen.c
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

# Очистка
clean:
	del /Q *.o *.a *.dll *.dll.a 2>nul || true

# Установка
install: all
	copy automator.h ..\..\include\ 2>nul || true
	copy $(STATIC_LIB) ..\..\lib\ 2>nul || true
	copy $(DYNAMIC_LIB) ..\..\lib\ 2>nul || true
	copy $(LIB_NAME).dll.a ..\..\lib\ 2>nul || true

.PHONY: all static dynamic clean install