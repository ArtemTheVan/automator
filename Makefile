CC = gcc
CFLAGS = -O2 -Wall -Wextra
LDFLAGS = -luser32 -lgdi32 # Библиотеки WinAPI для ввода и работы с графикой
TARGET = automator.exe
SOURCES = main.c
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
