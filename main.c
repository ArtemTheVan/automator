#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "simulate_keystroke.h"
#include "simulate_mouse.h"

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

int main()
{
    printf("=== GUI Automation Demo Started ===\n");
    // Даем пользователю время переключиться в нужное окно (5 секунд)
    printf("Switch to the target window within 5 seconds...\n");
    Sleep(5000);
    // Выполняем демонстрационные действия
    simulate_keystroke("hello!");
    // printf("=== Mouse Automation Demo ===\n");
    Sleep(3000);
    // Пример 1: Нажатие и отпускание левой кнопки на координатах (500, 300)
    printf("\n1. Simple click at (500, 300)\n");
    simulate_mouse_click_at(500, 300);
    // Пример 2: Произвольная последовательность с использованием MouseAction
    printf("\n2. Complex sequence: Move -> Click -> Drag -> Release\n");
    MouseAction complexSequence[] = {
        {500, 300, MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE}, // Перемещение
        {-1, -1, MOUSEEVENTF_LEFTDOWN},                      // Нажатие (координаты -1 = не менять)
        {700, 400, MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE}, // Перетаскивание
        {-1, -1, MOUSEEVENTF_LEFTUP}                         // Отпускание
    };
    // Пересчитываем координаты в абсолютный формат
    for (int i = 0; i < 4; i++)
    {
        if (complexSequence[i].x != -1)
        {
            complexSequence[i].x = (complexSequence[i].x * 65535) / GetSystemMetrics(SM_CXSCREEN);
            complexSequence[i].y = (complexSequence[i].y * 65535) / GetSystemMetrics(SM_CYSCREEN);
        }
    }
    simulate_mouse_sequence(complexSequence, 4);
    // // Пример 3: Двойной клик
    // printf("\n3. Double click at (600, 350)\n");
    // MouseAction doubleClick[] = {
    //     {600, 350, MOUSEEVENTF_LEFTDOWN},
    //     {-1, -1, MOUSEEVENTF_LEFTUP},
    //     {-1, -1, MOUSEEVENTF_LEFTDOWN},
    //     {-1, -1, MOUSEEVENTF_LEFTUP}
    // };
    // simulate_mouse_sequence(doubleClick, 4);
    // Захватываем небольшую область (пример: 100x100 пикселей от точки 500, 500)
    if (capture_screen_region(500, 500, 100, 100, "screenshot.bmp"))
    {
        printf("Screenshot saved. This can be used for OCR later.\n");
    }

    printf("=== Demo Finished ===\n");
    return 0;
}
