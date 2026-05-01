# console/Makefile - Консольное приложение
#
# Окружение: MSYS2 (mingw64).

CC ?= gcc
CFLAGS = -O2 -Wall -Wextra -I../lib
LDFLAGS = -L../lib -lautomator -luser32 -lgdi32 -lole32 -loleaut32 \
          -mconsole -Wl,--subsystem,console

TARGET = automator_console.exe
SOURCES = main.c
OBJECTS = $(SOURCES:.c=.o)

LIB_DEP = ../lib/libautomator.dll

all: $(LIB_DEP) $(TARGET)

$(TARGET): $(OBJECTS) $(LIB_DEP)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(LIB_DEP):
	@echo "Building library..."
	$(MAKE) -C ../lib dynamic

clean:
	@echo "Cleaning console..."
	@rm -f $(TARGET) *.o

run: $(TARGET)
	@echo "Running console..."
	@./$(TARGET)

test: $(TARGET)
	@echo "Running tests..."
	@./$(TARGET)

.PHONY: all clean run test
