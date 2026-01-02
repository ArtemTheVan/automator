QT += widgets

# Настройки проекта
TARGET = automator_qt
TEMPLATE = app
CONFIG += c++17

# Пути к библиотеке automator
INCLUDEPATH += ../lib
LIBS += -L../lib -lautomator

# Платформо-зависимые настройки
win32 {
    # Библиотеки WinAPI для функций эмуляции
    LIBS += -luser32 -lgdi32
    
    # Дополнительные флаги для Windows
    DEFINES += _WIN32_WINNT=0x0A00  # Windows 10
    QMAKE_LFLAGS += -static-libgcc -static-libstdc++
}

# Файлы проекта
SOURCES += main.cpp \
           automatorwidget.cpp

HEADERS += automatorwidget.h

# Настройки компилятора
QMAKE_CXXFLAGS += -Wall -Wextra
QMAKE_CFLAGS += -Wall -Wextra

# Отключаем некоторые предупреждения
QMAKE_CXXFLAGS += -Wno-unused-parameter