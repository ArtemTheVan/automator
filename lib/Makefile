# lib/Makefile - Сборка библиотеки libautomator
#
# Окружение: MSYS2 (mingw64). Cmd.exe и nmake не поддерживаются.
# Версия библиотеки задаётся через AUTOMATOR_VERSION (см. Makefile в корне).

CC ?= gcc
CXX ?= g++
AR ?= ar

AUTOMATOR_VERSION ?= 0.2.0

CFLAGS = -O2 -Wall -Wextra -I. -fPIC
CFLAGS += -DBUILDING_AUTOMATOR_DLL
CFLAGS += -DAUTOMATOR_VERSION=\"$(AUTOMATOR_VERSION)\"
CXXFLAGS = -O2 -Wall -Wextra -I. -fPIC -std=c++11
CXXFLAGS += -DBUILDING_AUTOMATOR_DLL
CXXFLAGS += -DAUTOMATOR_VERSION=\"$(AUTOMATOR_VERSION)\"
LDFLAGS_DLL = -shared -Wl,--out-implib,libautomator.dll.a

# OpenCV (опционально через pkg-config)
PKG_CONFIG ?= pkg-config
OPENCV_CFLAGS := $(shell $(PKG_CONFIG) --cflags opencv4 2>/dev/null)
OPENCV_LIBS   := $(shell $(PKG_CONFIG) --libs opencv4 2>/dev/null)

ifneq ($(OPENCV_CFLAGS),)
    HAVE_OPENCV = 1
    OPENCV_CFLAGS += -DHAVE_OPENCV
else ifneq ($(wildcard /mingw64/include/opencv4/opencv2/opencv.hpp),)
    HAVE_OPENCV = 1
    OPENCV_CFLAGS = -I/mingw64/include/opencv4 -DHAVE_OPENCV
    OPENCV_LIBS  = -L/mingw64/lib \
                   -lopencv_core -lopencv_imgproc \
                   -lopencv_highgui -lopencv_imgcodecs
else
    HAVE_OPENCV = 0
    OPENCV_CFLAGS =
    OPENCV_LIBS =
    $(warning OpenCV not found via pkg-config; building without OpenCV support.)
endif

CXXFLAGS += $(OPENCV_CFLAGS)

LIB_NAME = libautomator
STATIC_LIB = $(LIB_NAME).a
DYNAMIC_LIB = $(LIB_NAME).dll
DLL_IMPLIB = $(LIB_NAME).dll.a

C_SOURCES = init.c keyboard.c mouse.c screen.c ocr.c window.c
CXX_SOURCES = opencv_wrapper.cpp

C_OBJECTS = $(C_SOURCES:.c=.o)
CXX_OBJECTS = $(CXX_SOURCES:.cpp=.o)
OBJECTS = $(C_OBJECTS) $(CXX_OBJECTS)

HEADERS = automator.h keyboard.h mouse.h screen.h opencv.h ocr.h window.h log.h

all: static dynamic

static: $(STATIC_LIB)

dynamic: $(DYNAMIC_LIB)

$(STATIC_LIB): $(OBJECTS)
	$(AR) rcs $@ $^

$(DYNAMIC_LIB): $(OBJECTS)
	$(CXX) $(LDFLAGS_DLL) -o $@ $(OBJECTS) -lgdi32 -luser32 -lole32 -loleaut32 $(OPENCV_LIBS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@echo "Cleaning library..."
	@rm -f $(STATIC_LIB) $(DYNAMIC_LIB) $(DLL_IMPLIB) *.o

.PHONY: all static dynamic clean
