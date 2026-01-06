#ifndef WINDOW_H_
#define WINDOW_H_

#include <windows.h>

/* Макросы для экспорта функций из DLL */
#ifdef BUILDING_AUTOMATOR_DLL /* ИЗМЕНЕНО с BUILDING_WINDOW_DLL */
#define WINDOW_API __declspec(dllexport)
#else
#define WINDOW_API __declspec(dllimport)
#endif

/* Структура для информации об окне */
typedef struct
{
    HWND handle;          /* Дескриптор окна */
    char title[256];      /* Заголовок окна */
    char class_name[256]; /* Имя класса окна */
    RECT rect;            /* Прямоугольник окна */
    int pid;              /* ID процесса */
    BOOL is_visible;      /* Видимо ли окно */
} WindowInfo;

/* Структура для списка окон */
typedef struct
{
    WindowInfo *windows;
    int count;
    int capacity;
} WindowList;

/* Функции для работы с окнами */
WINDOW_API HWND find_virtual_machine_window(void);
WINDOW_API char *get_window_text_ex(HWND hwnd, char *buffer, int buffer_size);

/* Эти функции тоже нужны для внутреннего использования */
WINDOW_API WindowList *find_all_visible_windows(void);
WINDOW_API void free_window_list(WindowList *list);
WINDOW_API HWND find_system_tray_window(void);

#endif