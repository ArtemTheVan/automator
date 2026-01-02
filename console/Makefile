# console/Makefile - Консольное приложение

CC = gcc
CFLAGS = -O2 -Wall -Wextra -I../lib
LDFLAGS = -L../lib -lautomator -luser32 -lgdi32

TARGET = automator_console.exe
SOURCES = main.c
OBJECTS = $(SOURCES:.c=.o)
LIB_DEP = ../lib/libautomator.a

all: $(LIB_DEP) $(TARGET)

$(TARGET): $(OBJECTS) $(LIB_DEP)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(LIB_DEP):
	@echo "Сборка библиотеки..."
	cd ../lib && $(MAKE) static

clean:
	del /Q *.o $(TARGET) 2>nul || true

run: $(TARGET)
	@echo "Запуск консольной версии..."
	.\$(TARGET)

test: $(TARGET)
	@echo "Запуск тестов..."
	.\$(TARGET)

.PHONY: all clean run test