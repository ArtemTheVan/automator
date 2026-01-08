# console/Makefile - Консольное приложение

CC = gcc
CFLAGS = -O2 -Wall -Wextra -I../lib
LDFLAGS = -L../lib -lautomator -luser32 -lgdi32 -lole32 -loleaut32 -mconsole -Wl,--subsystem,console

TARGET = automator_console.exe
SOURCES = main.c
OBJECTS = $(SOURCES:.c=.o)

# Подпроекты
LIB_DEP = ../lib/libautomator.dll

all: $(LIB_DEP) $(TARGET)

$(TARGET): $(OBJECTS) $(LIB_DEP)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(LIB_DEP):
	@echo "Сборка библиотеки..."
	cd ../lib && $(MAKE) dynamic

clean:
	@echo "Очистка проекта..."
	-@rm -f $(TARGET) 2>/dev/null || true
	-@rm -f *.o 2>/dev/null || true
	@echo "Очистка завершена"

run: $(TARGET)
	@echo "Запуск консольной версии..."
	@./$(TARGET)

test: $(TARGET)
	@echo "Запуск тестов..."
	@./$(TARGET)

.PHONY: all clean run test