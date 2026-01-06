#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "screen.h"

/* Вспомогательные функции */
static HBITMAP create_bitmap_from_region(int x, int y, int width, int height)
{
    HDC hdcScreen = GetDC(NULL);
    if (!hdcScreen)
        return NULL;

    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);

    if (hBitmap)
    {
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);
        BitBlt(hdcMem, 0, 0, width, height, hdcScreen, x, y, SRCCOPY);
        SelectObject(hdcMem, hOldBitmap);
    }

    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);

    return hBitmap;
}

/* Основные функции захвата экрана */
int capture_screen_region(int x, int y, int width, int height, const char *filename)
{
    HBITMAP hBitmap = create_bitmap_from_region(x, y, width, height);
    if (!hBitmap)
        return 0;

    int result = save_bitmap_to_file(hBitmap, filename);
    DeleteObject(hBitmap);

    printf("Screen region captured: %s\n", filename);
    return result;
}

int capture_active_window(const char *filename)
{
    HWND hwnd = GetForegroundWindow();
    if (!hwnd)
        return 0;

    RECT rect;
    GetWindowRect(hwnd, &rect);

    return capture_screen_region(
        rect.left, rect.top,
        rect.right - rect.left,
        rect.bottom - rect.top,
        filename);
}

HBITMAP capture_to_bitmap(int x, int y, int width, int height)
{
    return create_bitmap_from_region(x, y, width, height);
}

int save_bitmap_to_file(HBITMAP hBitmap, const char *filename)
{
    BITMAP bmp;
    BITMAPFILEHEADER bmfHeader;
    BITMAPINFOHEADER bi;
    DWORD dwBmpSize;
    HANDLE hFile;
    char *lpbitmap = NULL;
    HDC hDC;
    DWORD dwSizeofDIB, dwBytesWritten;

    /* Получаем информацию о битмапе */
    if (!GetObject(hBitmap, sizeof(BITMAP), &bmp))
    {
        printf("Failed to get bitmap info\n");
        return 0;
    }

    printf("Bitmap dimensions: %ldx%ld, bpp: %d\n", bmp.bmWidth, bmp.bmHeight, bmp.bmBitsPixel);

    /* Проверяем размеры */
    if (bmp.bmWidth <= 0 || bmp.bmHeight <= 0)
    {
        printf("Invalid bitmap dimensions: %ldx%ld\n", bmp.bmWidth, bmp.bmHeight);
        return 0;
    }

    /* Заполняем BITMAPINFOHEADER */
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmp.bmWidth;
    bi.biHeight = -bmp.bmHeight; /* Положительное значение для bottom-up DIB */
    bi.biPlanes = 1;
    bi.biBitCount = 24; /* 24-битный формат */
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    /* Вычисляем размер пиксельных данных */
    /* Ширина в байтах должна быть кратна 4 */
    DWORD lineSize = ((bmp.bmWidth * 24 + 31) / 32) * 4;
    dwBmpSize = lineSize * bmp.bmHeight;

    printf("Bitmap size calculation: lineSize=%lu, dwBmpSize=%lu\n", lineSize, dwBmpSize);

    /* Выделяем память для пиксельных данных */
    lpbitmap = (char *)malloc(dwBmpSize);
    if (!lpbitmap)
    {
        printf("Failed to allocate memory for bitmap data\n");
        return 0;
    }

    /* Получаем контекст устройства */
    hDC = GetDC(NULL);
    if (!hDC)
    {
        printf("Failed to get device context\n");
        free(lpbitmap);
        return 0;
    }

    /* Получаем данные битмапа */
    if (!GetDIBits(hDC, hBitmap, 0, bmp.bmHeight, lpbitmap,
                   (BITMAPINFO *)&bi, DIB_RGB_COLORS))
    {
        printf("GetDIBits failed, error: %lu\n", GetLastError());
        free(lpbitmap);
        ReleaseDC(NULL, hDC);
        return 0;
    }

    ReleaseDC(NULL, hDC);

    /* Заполняем BITMAPFILEHEADER */
    dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);
    bmfHeader.bfSize = dwSizeofDIB;
    bmfHeader.bfType = 0x4D42; /* "BM" */

    /* Создаем файл */
    hFile = CreateFile(filename, GENERIC_WRITE, 0, NULL,
                       CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        printf("Failed to create file %s, error: %lu\n", filename, GetLastError());
        free(lpbitmap);
        return 0;
    }

    /* Записываем данные в файл */
    BOOL writeSuccess = TRUE;

    if (!WriteFile(hFile, &bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL) ||
        dwBytesWritten != sizeof(BITMAPFILEHEADER))
    {
        printf("Failed to write bitmap file header\n");
        writeSuccess = FALSE;
    }

    if (writeSuccess &&
        (!WriteFile(hFile, &bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL) ||
         dwBytesWritten != sizeof(BITMAPINFOHEADER)))
    {
        printf("Failed to write bitmap info header\n");
        writeSuccess = FALSE;
    }

    if (writeSuccess &&
        (!WriteFile(hFile, lpbitmap, dwBmpSize, &dwBytesWritten, NULL) ||
         dwBytesWritten != dwBmpSize))
    {
        printf("Failed to write bitmap data\n");
        writeSuccess = FALSE;
    }

    /* Закрываем файл и освобождаем память */
    CloseHandle(hFile);
    free(lpbitmap);

    if (writeSuccess)
    {
        printf("Bitmap saved successfully: %s (%ldx%ld)\n", filename, bmp.bmWidth, bmp.bmHeight);
        return 1;
    }
    else
    {
        DeleteFile(filename); /* Удаляем частично записанный файл */
        return 0;
    }
}

/* Утилиты для работы с экраном */
int get_screen_width()
{
    return GetSystemMetrics(SM_CXSCREEN);
}

int get_screen_height()
{
    return GetSystemMetrics(SM_CYSCREEN);
}

/* Старая версия (оставляем для совместимости) */
ScreenRegion get_system_tray_region()
{
    ScreenRegion region = {0};

    int screen_width = get_screen_width();
    int screen_height = get_screen_height();

    /* Увеличим область для лучшего захвата */
    region.width = 120;                            /* Ширина */
    region.height = 50;                            /* Высота */
    region.x = screen_width - region.width - 90;   /* Правая часть */
    region.y = screen_height - region.height - 10; /* Нижняя часть */

    printf("System tray region (fixed): %dx%d at (%d,%d)\n",
           region.width, region.height, region.x, region.y);

    return region;
}

/* В конце screen.c, после get_system_tray_region, добавляем: */

/* Функция автовыбора метода */
ScreenRegion get_system_tray_region_auto(void)
{
    ScreenRegion region;

/* Пытаемся использовать OpenCV, если он доступен */
#ifdef USE_OPENCV
    DetectedRegion detected = detect_system_tray_region();
    region.x = detected.x;
    region.y = detected.y;
    region.width = detected.width;
    region.height = detected.height;

    /* Если OpenCV не нашел, используем старый метод */
    if (region.width == 0 || region.height == 0)
    {
        region = get_system_tray_region();
    }
#else
    region = get_system_tray_region();
#endif

    return region;
}