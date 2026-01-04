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
    BITMAPFILEHEADER bmfHeader;
    BITMAPINFOHEADER bi;
    DWORD dwBmpSize;

    HDC hDC = CreateCompatibleDC(NULL);
    GetDIBits(hDC, hBitmap, 0, 0, NULL, (BITMAPINFO *)&bi, DIB_RGB_COLORS);

    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bi.biWidth;
    bi.biHeight = -abs(bi.biHeight);
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = BI_RGB;

    dwBmpSize = ((bi.biWidth * bi.biBitCount + 31) / 32) * 4 * abs(bi.biHeight);
    HANDLE hDIB = GlobalAlloc(GHND, dwBmpSize);

    if (!hDIB)
    {
        DeleteDC(hDC);
        return 0;
    }

    char *lpbitmap = (char *)GlobalLock(hDIB);
    GetDIBits(hDC, hBitmap, 0, abs(bi.biHeight), lpbitmap, (BITMAPINFO *)&bi, DIB_RGB_COLORS);

    DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmfHeader.bfSize = dwSizeofDIB;
    bmfHeader.bfType = 0x4D42;

    FILE *fp = fopen(filename, "wb");
    if (!fp)
    {
        GlobalFree(hDIB);
        DeleteDC(hDC);
        return 0;
    }

    fwrite(&bmfHeader, sizeof(BITMAPFILEHEADER), 1, fp);
    fwrite(&bi, sizeof(BITMAPINFOHEADER), 1, fp);
    fwrite(lpbitmap, 1, dwBmpSize, fp);

    fclose(fp);
    GlobalFree(hDIB);
    DeleteDC(hDC);
    return 1;
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

ScreenRegion get_system_tray_region()
{
    ScreenRegion region = {0};

    region.width = 200;
    region.height = 100;
    region.x = get_screen_width() - region.width - 10;
    region.y = get_screen_height() - region.height - 10;

    return region;
}