# lib/Makefile - Сборка библиотеки libautomator

CC = gcc
CXX = g++  # Для компиляции C++ оберток
AR = ar
CFLAGS = -O2 -Wall -Wextra -I. -fPIC
CFLAGS += -DBUILDING_AUTOMATOR_DLL
CXXFLAGS = -O2 -Wall -Wextra -I. -fPIC
CXXFLAGS += -DBUILDING_AUTOMATOR_DLL
LDFLAGS_DLL = -shared -Wl,--out-implib,libautomator.dll.a

# Имена файлов
LIB_NAME = libautomator
STATIC_LIB = $(LIB_NAME).a
DYNAMIC_LIB = $(LIB_NAME).dll
DLL_IMPLIB = $(LIB_NAME).dll.a

# Исходные файлы
C_SOURCES = keyboard.c mouse.c screen.c ocr.c window.c
CXX_SOURCES = opencv_wrapper.cpp  # C++ обертка

C_OBJECTS = $(C_SOURCES:.c=.o)
CXX_OBJECTS = $(CXX_SOURCES:.cpp=.o)
OBJECTS = $(C_OBJECTS) $(CXX_OBJECTS)

HEADERS = automator.h screen.h

# По умолчанию собираем обе библиотеки
all: static dynamic

# Статическая библиотека
static: $(STATIC_LIB)

# Динамическая библиотека
dynamic: $(DYNAMIC_LIB)

$(STATIC_LIB): $(OBJECTS)
	$(AR) rcs $@ $^

$(DYNAMIC_LIB): $(OBJECTS)
	$(CXX) $(LDFLAGS_DLL) -o $@ $(OBJECTS) -lgdi32 -luser32 -lole32 -loleaut32

# Компиляция C исходников
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Компиляция C++ исходников
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Очистка
clean:
	@echo "Cleaning library..."
	@rm -f $(STATIC_LIB) $(DYNAMIC_LIB) $(DLL_IMPLIB) *.o 2>/dev/null || true
	@echo "Cleanup complete"

.PHONY: all static dynamic clean