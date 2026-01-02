#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "libautomator.h"

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
