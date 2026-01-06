#ifndef SCREEN_H_
#define SCREEN_H_

#include <windows.h>

/* Макросы для экспорта функций из DLL */
#ifdef BUILDING_AUTOMATOR_DLL /* ИЗМЕНЕНО с BUILDING_SCREEN_DLL */
#define SCREEN_API __declspec(dllexport)
#else
#define SCREEN_API __declspec(dllimport)
#endif

/* Структура для области экрана */
typedef struct
{
    int x;
    int y;
    int width;
    int height;
} ScreenRegion;

/* Функции захвата экрана */
SCREEN_API int capture_screen_region(int x, int y, int width, int height, const char *filename);
SCREEN_API HBITMAP capture_to_bitmap(int x, int y, int width, int height);
SCREEN_API int save_bitmap_to_file(HBITMAP hBitmap, const char *filename);

/* Утилиты для работы с экраном */
SCREEN_API int get_screen_width(void);
SCREEN_API int get_screen_height(void);

/* Функции для определения системного трея */
SCREEN_API ScreenRegion get_system_tray_region(void);
SCREEN_API ScreenRegion get_system_tray_region_auto(void);

#endif