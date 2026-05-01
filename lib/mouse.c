#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "mouse.h"
#include "log.h"

/* Задаётся в init.c, переопределяется через set_mouse_delay_ms(). */
extern int g_mouse_delay_ms;

/*
 * Преобразует абсолютные пиксельные координаты на виртуальном экране
 * (объединяющем все мониторы) в систему координат, ожидаемую SendInput
 * с флагом MOUSEEVENTF_VIRTUALDESK (0..65535 на весь виртуальный экран).
 */
static void to_virtual_desk_coords(int x, int y, LONG *out_dx, LONG *out_dy)
{
    int vx = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int vy = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int vw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int vh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    if (vw <= 0) vw = 1;
    if (vh <= 0) vh = 1;

    /* MOUSEEVENTF_VIRTUALDESK ожидает координаты в диапазоне 0..65535,
       линейно отображённые на виртуальный рабочий стол. */
    *out_dx = (LONG)(((LONGLONG)(x - vx) * 65535) / vw);
    *out_dy = (LONG)(((LONGLONG)(y - vy) * 65535) / vh);
}

void simulate_mouse_sequence(const MouseAction *actions, int count)
{
    if (!actions || count <= 0) {
        LOG_ERROR("Invalid mouse action sequence");
        return;
    }
    LOG_DEBUG("Simulating mouse sequence (%d actions)", count);

    for (int i = 0; i < count; i++) {
        MouseAction action = actions[i];
        INPUT ip = {0};
        ip.type = INPUT_MOUSE;

        /* Координаты со значением -1 означают «не двигать мышь». */
        if (action.x != -1 && action.y != -1) {
            LONG dx = 0, dy = 0;
            to_virtual_desk_coords(action.x, action.y, &dx, &dy);
            ip.mi.dx = dx;
            ip.mi.dy = dy;
            ip.mi.dwFlags |= MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK | MOUSEEVENTF_MOVE;
        }
        ip.mi.dwFlags |= action.dwFlags;
        SendInput(1, &ip, sizeof(INPUT));

        if (g_mouse_delay_ms > 0) {
            Sleep(g_mouse_delay_ms);
        }
    }
}

void simulate_mouse_click_at(int x, int y)
{
    MouseAction actions[] = {
        {x, y, MOUSEEVENTF_LEFTDOWN},
        {x, y, MOUSEEVENTF_LEFTUP}
    };
    simulate_mouse_sequence(actions, 2);
}
