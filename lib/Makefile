# lib/Makefile - Сборка библиотеки libautomator

CC = gcc
CXX = g++
AR = ar
CFLAGS = -O2 -Wall -Wextra -I. -fPIC
CFLAGS += -DBUILDING_AUTOMATOR_DLL
CXXFLAGS = -O2 -Wall -Wextra -I. -fPIC -std=c++11
CXXFLAGS += -DBUILDING_AUTOMATOR_DLL
LDFLAGS_DLL = -shared -Wl,--out-implib,libautomator.dll.a

# Используем pkg-config для OpenCV
PKG_CONFIG = pkg-config
OPENCV_CFLAGS := $(shell $(PKG_CONFIG) --cflags opencv4 2>/dev/null || echo "")
OPENCV_LIBS := $(shell $(PKG_CONFIG) --libs opencv4 2>/dev/null || echo "")

# Проверяем, был ли найден OpenCV через pkg-config
ifneq ($(OPENCV_CFLAGS),)
    # OpenCV найден через pkg-config
    OPENCV_CFLAGS += -DHAVE_OPENCV
    HAVE_OPENCV = 1
else
    # Проверяем стандартные пути MSYS2
    ifeq ($(wildcard /mingw64/include/opencv4/opencv2/opencv.hpp),)
        ifeq ($(wildcard /usr/include/opencv4/opencv2/opencv.hpp),)
            $(warning OpenCV not found. Text detection will be limited.)
            HAVE_OPENCV = 0
        else
            # OpenCV 4.x в /usr/include
            OPENCV_CFLAGS = -I/usr/include/opencv4 -DHAVE_OPENCV
            OPENCV_LIBS = -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs -lopencv_dnn -lopencv_text
            HAVE_OPENCV = 1
        endif
    else
        # OpenCV 4.x в /mingw64/include
        OPENCV_CFLAGS = -I/mingw64/include/opencv4 -DHAVE_OPENCV
        OPENCV_LIBS = -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs -lopencv_dnn -lopencv_text
        HAVE_OPENCV = 1
    endif
endif

# Добавляем флаги OpenCV
CXXFLAGS += $(OPENCV_CFLAGS)

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

HEADERS = automator.h screen.h opencv.h ocr.h

# По умолчанию собираем обе библиотеки
all: static dynamic

# Статическая библиотека
static: $(STATIC_LIB)

# Динамическая библиотека
dynamic: $(DYNAMIC_LIB)

$(STATIC_LIB): $(OBJECTS)
	$(AR) rcs $@ $^

$(DYNAMIC_LIB): $(OBJECTS)
	$(CXX) $(LDFLAGS_DLL) -o $@ $(OBJECTS) -lgdi32 -luser32 -lole32 -loleaut32 $(OPENCV_LIBS)

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