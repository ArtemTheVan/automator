#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include <windows.h>
#include <stdio.h>
#include <string.h>

/* Макросы для экспорта функций из DLL */
#ifdef BUILDING_AUTOMATOR_DLL
#define KEYBOARD_API __declspec(dllexport)
#else
#define KEYBOARD_API __declspec(dllimport)
#endif

// Симуляция нажатия клавиш для строки (включая спецсимволы)
KEYBOARD_API void simulate_keystroke(const char *text);

#endif
