#ifndef SCREEN_H_
#define SCREEN_H_

#include <windows.h>
#include <stdio.h>
#include <string.h>

/* Структура для области экрана */
typedef struct
{
    int x;
    int y;
    int width;
    int height;
} ScreenRegion;

/* Функции захвата экрана */
int capture_screen_region(int x, int y, int width, int height, const char *filename);
int capture_active_window(const char *filename);
HBITMAP capture_to_bitmap(int x, int y, int width, int height);
int save_bitmap_to_file(HBITMAP hBitmap, const char *filename);

/* Утилиты для работы с экраном */
int get_screen_width();
int get_screen_height();
ScreenRegion get_system_tray_region();

#endif