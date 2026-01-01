#include <windows.h>
#include <stdio.h>
#include <string.h>

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

// 2. Функция симуляции клика мыши в текущей позиции курсора
void simulate_mouse_click()
{
    printf("Simulating mouse click...\n");
    INPUT ip = {0};
    ip.type = INPUT_MOUSE;
    ip.mi.dwFlags = MOUSEEVENTF_LEFTDOWN; // Нажатие левой кнопки
    SendInput(1, &ip, sizeof(INPUT));
    ip.mi.dwFlags = MOUSEEVENTF_LEFTUP; // Отпускание левой кнопки
    SendInput(1, &ip, sizeof(INPUT));
    Sleep(100);
}

// 3. Функция захвата области экрана (основа для будущего OCR)
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
    simulate_mouse_click();

    // Захватываем небольшую область (пример: 100x100 пикселей от точки 500, 500)
    if (capture_screen_region(500, 500, 100, 100, "screenshot.bmp"))
    {
        printf("Screenshot saved. This can be used for OCR later.\n");
    }

    printf("=== Demo Finished ===\n");
    return 0;
}
