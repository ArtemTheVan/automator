# lib/Makefile - Сборка библиотеки libautomator

CC = gcc
AR = ar
CFLAGS = -O2 -Wall -Wextra -I. -fPIC
LDFLAGS = -luser32 -lgdi32

# Имена файлов
LIB_NAME = libautomator
STATIC_LIB = $(LIB_NAME).a
DYNAMIC_LIB = $(LIB_NAME).dll

# Исходные файлы
SOURCES = keyboard.c mouse.c screen.c
OBJECTS = $(SOURCES:.c=.o)
HEADERS = automator.h

# По умолчанию собираем статическую библиотеку
all: static

# Статическая библиотека
static: $(STATIC_LIB)

# Динамическая библиотека (опционально)
dynamic: $(DYNAMIC_LIB)

$(STATIC_LIB): $(OBJECTS)
	$(AR) rcs $@ $^

$(DYNAMIC_LIB): $(OBJECTS)
	$(CC) -shared -o $@ $(OBJECTS) $(LDFLAGS) -Wl,--out-implib,$(LIB_NAME).dll.a

# Компиляция исходников
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Очистка
clean:
	del /Q *.o *.a *.dll *.dll.a 2>nul || true

# Установка
install: $(STATIC_LIB)
	copy automator.h ..\..\include\ 2>nul || true
	copy $(STATIC_LIB) ..\..\lib\ 2>nul || true

.PHONY: all static dynamic clean install