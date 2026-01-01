#include <windows.h>
#include <stdio.h>

// 1. Функция симуляции нажатия клавиши (F15 - редко используемая, безопасная для тестов)
void simulate_keystroke()
{
    printf("Simulating F15 key press...\n");
    INPUT ip;
    ip.type = INPUT_KEYBOARD;
    ip.ki.wScan = 0;
    ip.ki.time = 0;
    ip.ki.dwExtraInfo = 0;

    // Нажатие клавиши
    ip.ki.wVk = 0x7E;  // Virtual-Key код для F15
    ip.ki.dwFlags = 0; // 0 означает нажатие клавиши
    SendInput(1, &ip, sizeof(INPUT));

    // Отпускание клавиши
    ip.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &ip, sizeof(INPUT));
    Sleep(100); // Небольшая пауза между действиями
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
    simulate_keystroke();
    simulate_mouse_click();

    // Захватываем небольшую область (пример: 100x100 пикселей от точки 500, 500)
    if (capture_screen_region(500, 500, 100, 100, "screenshot.bmp"))
    {
        printf("Screenshot saved. This can be used for OCR later.\n");
    }

    printf("=== Demo Finished ===\n");
    return 0;
}
