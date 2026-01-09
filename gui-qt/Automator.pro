
QT += core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# Настройки проекта
TARGET = automator_qt
TEMPLATE = app
CONFIG += c++17

# Пути к библиотекам
INCLUDES += ../lib

# Платформо-зависимые настройки
win32 {
    # Библиотеки WinAPI для функций эмуляции
    LIBS += -luser32 -lgdi32
    
    # Дополнительные флаги для Windows
    DEFINES += _WIN32_WINNT=0x0A00  # Windows 10
    QMAKE_LFLAGS += -static-libgcc -static-libstdc++
}

# Файлы проекта
SOURCES += \
    main.cpp \
    automatorwidget.cpp \
    settingsdialog.cpp

HEADERS += \
    automatorwidget.h \
    settingsdialog.h

# Настройки компилятора
QMAKE_CXXFLAGS += -Wall -Wextra
QMAKE_CFLAGS += -Wall -Wextra

# Отключаем некоторые предупреждения
QMAKE_CXXFLAGS += -Wno-unused-parameter