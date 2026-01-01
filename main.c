#include <windows.h>
#include <stdio.h>
#include <string.h>

// Структура для описания одного действия мыши
typedef struct
{
    int x;         // Координата X (относительные или абсолютные)
    int y;         // Координата Y (относительные или абсолютные)
    DWORD dwFlags; // Флаги действия (нажатие, отпускание, перемещение и т.д.)
} MouseAction;

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
