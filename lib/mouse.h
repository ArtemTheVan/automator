#ifndef MOUSE_H_
#define MOUSE_H_

#include <windows.h>
#include <stdio.h>
#include <string.h>

/* Макросы для экспорта функций из DLL */
#ifdef BUILDING_AUTOMATOR_DLL
#define MOUSE_API __declspec(dllexport)
#else
#define MOUSE_API __declspec(dllimport)
#endif

// Структура для описания одного действия мыши
typedef struct
{
    int x;         // Координата X (относительные или абсолютные)
    int y;         // Координата Y (относительные или абсолютные)
    DWORD dwFlags; // Флаги действия (нажатие, отпускание, перемещение и т.д.)
} MouseAction;

// Функция эмуляции последовательности действий мыши
MOUSE_API void simulate_mouse_sequence(const MouseAction *actions, int count);

// Альтернативная функция с фиксированной последовательностью
MOUSE_API void simulate_mouse_click_at(int x, int y);

#endif
