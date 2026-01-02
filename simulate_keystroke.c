#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "simulate_keystroke.h"

// Симуляция нажатия клавиш для строки (включая спецсимволы)
void simulate_keystroke(const char *text)
{
    printf("Simulating keystrokes for: %s\n", text);
    for (size_t i = 0; i < strlen(text); i++)
    {
        char c = text[i];
        SHORT vk = 0;
        BOOL shift_needed = FALSE;
        // Определяем виртуальный код клавиши и необходимость в Shift
        if (c >= 'a' && c <= 'z')
        {
            vk = 0x41 + (c - 'a'); // 0x41 = 'A', строчные без Shift
        }
        else if (c >= 'A' && c <= 'Z')
        {
            vk = 0x41 + (c - 'A'); // Заглавные требуют Shift
            shift_needed = TRUE;
        }
        else if (c >= '0' && c <= '9')
        {
            vk = 0x30 + (c - '0'); // 0x30 = '0', цифры без Shift
        }
        else
        {
            // Обработка специальных символов
            switch (c)
            {
            case '!':
                vk = 0x31; // Клавиша '1'
                shift_needed = TRUE;
                break;
            case '@':
                vk = 0x32; // Клавиша '2'
                shift_needed = TRUE;
                break;
            case ' ':
                vk = VK_SPACE;
                break;
            default:
                // Если символ не поддерживается, пропустить
                continue;
            }
        }
        // Нажатие Shift, если требуется
        if (shift_needed)
        {
            INPUT shift_down = {0};
            shift_down.type = INPUT_KEYBOARD;
            shift_down.ki.wVk = VK_SHIFT;
            SendInput(1, &shift_down, sizeof(INPUT));
        }
        // Нажатие и отпускание основной клавиши
        INPUT key_event = {0};
        key_event.type = INPUT_KEYBOARD;
        key_event.ki.wVk = vk;
        SendInput(1, &key_event, sizeof(INPUT));
        key_event.ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &key_event, sizeof(INPUT));
        // Отпускание Shift, если нажимали
        if (shift_needed)
        {
            INPUT shift_up = {0};
            shift_up.type = INPUT_KEYBOARD;
            shift_up.ki.wVk = VK_SHIFT;
            shift_up.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &shift_up, sizeof(INPUT));
        }
        Sleep(10); // Короткая пауза между символами
    }
}
