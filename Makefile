CC = gcc
CFLAGS = -O2 -Wall -Wextra -I.
LDFLAGS = -luser32 -lgdi32
TARGET = automator.exe

# Автоматический поиск всех .c файлов
SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	del $(OBJECTS) $(TARGET) 2>nul || true

run: $(TARGET)
	.\$(TARGET)

# Правило для очистки и пересборки
rebuild: clean all
