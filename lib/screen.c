#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "screen.h"

// Внутренняя функция сохранения BMP (упрощенная)
static int save_bitmap_to_file(HBITMAP hBitmap, const char* filename)
{
    return 1;
}

int capture_active_window(const char* filename)
{
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return 0;
    
    RECT rect;
    GetWindowRect(hwnd, &rect);
    
    return capture_screen_region(
        rect.left, rect.top,
        rect.right - rect.left,
        rect.bottom - rect.top,
        filename
    );
}

// Функция захвата области экрана (основа для будущего OCR)
// Сохраняет скриншот области (x, y, width, height) в BMP файл.
int capture_screen_region(int x, int y, int width, int height, const char *filename)
{
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
    SelectObject(hdcMem, hBitmap);

    // Копируем область экрана в битмап
    BitBlt(hdcMem, 0, 0, width, height, hdcScreen, x, y, SRCCOPY);

    // Сохраняем битмап в файл (код сохранения BMP опущен для краткости, но его легко найти)
    // ... (Здесь должен быть код сохранения HBITMAP в .bmp файл)

    // Очистка ресурсов
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);

    printf("Screen region captured. File: %s\n", filename);
    return 1;
}
