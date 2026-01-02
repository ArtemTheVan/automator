#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "simulate_mouse.h"

// Функция эмуляции последовательности действий мыши
void simulate_mouse_sequence(const MouseAction *actions, int count)
{
    if (!actions || count <= 0)
    {
        printf("Error: Invalid mouse action sequence\n");
        return;
    }
    printf("Simulating mouse sequence (%d actions)...\n", count);
    for (int i = 0; i < count; i++)
    {
        MouseAction action = actions[i];
        INPUT ip = {0};
        ip.type = INPUT_MOUSE;
        // Если координаты указаны (не -1), используем их
        if (action.x != -1 && action.y != -1)
        {
            // Преобразование в абсолютные координаты (0-65535)
            ip.mi.dx = (action.x * 65535) / GetSystemMetrics(SM_CXSCREEN);
            ip.mi.dy = (action.y * 65535) / GetSystemMetrics(SM_CYSCREEN);
            ip.mi.dwFlags |= MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
        }
        ip.mi.dwFlags |= action.dwFlags;
        SendInput(1, &ip, sizeof(INPUT));
        // Небольшая пауза между действиями
        Sleep(50);
    }
}

// Альтернативная функция с фиксированной последовательностью
void simulate_mouse_click_at(int x, int y)
{
    MouseAction actions[] = {
        {x, y, MOUSEEVENTF_LEFTDOWN}, // Нажатие левой кнопки
        {x, y, MOUSEEVENTF_LEFTUP}    // Отпускание левой кнопки
    };
    simulate_mouse_sequence(actions, 2);
}
