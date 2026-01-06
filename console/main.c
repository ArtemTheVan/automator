#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <ctype.h>
#include "automator.h"

/* OCR language constants */
#define OCR_LANG_ENGLISH "eng"
#define OCR_LANG_RUSSIAN "rus"
#define OCR_LANG_ENGLISH_RUSSIAN "eng+rus"
#define OCR_LANG_DEFAULT OCR_LANG_ENGLISH_RUSSIAN

/* Keyboard layout detection using Windows API */
int detect_layout_windows_api(void)
{
    HWND hwnd = GetForegroundWindow();
    if (!hwnd)
    {
        printf("No active window found\n");
        return -1;
    }

    DWORD threadId = GetWindowThreadProcessId(hwnd, NULL);
    HKL layout = GetKeyboardLayout(threadId);
    WORD langId = LOWORD(layout);

    printf("Windows API: Keyboard layout code: 0x%04X\n", langId);

    switch (langId)
    {
    case 0x0409:  /* English (United States) */
    case 0x0809:  /* English (United Kingdom) */
    case 0x0C09:  /* English (Australian) */
    case 0x1009:  /* English (Canadian) */
    case 0x1409:  /* English (New Zealand) */
    case 0x1809:  /* English (Irish) */
    case 0x1C09:  /* English (South Africa) */
        return 0; /* ENG */

    case 0x0419:  /* Russian */
    case 0x0819:  /* Russian (Moldova) */
        return 1; /* RUS */

    default:
        printf("Unknown layout code: 0x%04X\n", langId);
        return -2;
    }
}

void save_image_for_debug(ScreenRegion region, const char *name)
{
    char filename[256];
    sprintf(filename, "debug_%s_%lu.bmp", name, (unsigned long)GetCurrentProcessId());

    if (capture_screen_region(region.x, region.y, region.width, region.height, filename))
    {
        printf("Debug image saved: %s\n", filename);
    }
}

/* Keyboard layout detection using OCR с OpenCV */
int detect_layout_ocr_opencv(void)
{
    printf("\n=== OCR with OpenCV Method ===\n");

    if (ocr_init(NULL) != OCR_SUCCESS)
    {
        printf("OCR initialization failed\n");
        return -1;
    }

    /* Инициализируем OpenCV */
    printf("Initializing OpenCV...\n");
    if (!opencv_init())
    {
        printf("OpenCV initialization failed\n");
        ocr_cleanup();
        return -1;
    }

    /* Используем OpenCV для определения региона системного трея */
    printf("Detecting system tray region...\n");
    DetectedRegion tray_region = detect_system_tray_region_opencv();

    if (tray_region.width == 0 || tray_region.height == 0)
    {
        printf("ERROR: Failed to detect system tray with OpenCV\n");
        opencv_cleanup();
        ocr_cleanup();
        return -1;
    }

    printf("SUCCESS: OpenCV detected tray region: %dx%d at (%d,%d), confidence: %.2f\n",
           tray_region.width, tray_region.height, tray_region.x, tray_region.y,
           tray_region.confidence);

    /* Преобразуем DetectedRegion в ScreenRegion */
    ScreenRegion screen_region;
    screen_region.x = tray_region.x;
    screen_region.y = tray_region.y;
    screen_region.width = tray_region.width;
    screen_region.height = tray_region.height;

    /* Пытаемся найти текст раскладки внутри региона трея */
    printf("Looking for keyboard layout text...\n");
    DetectedRegion layout_region = find_keyboard_layout_in_region(screen_region);

    ScreenRegion ocr_region;

    if (layout_region.width > 0 && layout_region.height > 0)
    {
        /* ВМЕСТО использования найденной области напрямую,
           увеличим ее для лучшего захвата OCR */
        int padding_x = 60; /* Добавим 60 пикселей по горизонтали */
        int padding_y = 30; /* Добавим 30 пикселей по вертикали */

        ocr_region.x = layout_region.x - padding_x;
        ocr_region.y = layout_region.y - padding_y;
        ocr_region.width = layout_region.width + 2 * padding_x;
        ocr_region.height = layout_region.height + 2 * padding_y;

        /* Убедимся, что не вышли за границы экрана */
        int screen_width = get_screen_width();
        int screen_height = get_screen_height();

        if (ocr_region.x < 0)
            ocr_region.x = 0;
        if (ocr_region.y < 0)
            ocr_region.y = 0;
        if (ocr_region.x + ocr_region.width > screen_width)
            ocr_region.width = screen_width - ocr_region.x;
        if (ocr_region.y + ocr_region.height > screen_height)
            ocr_region.height = screen_height - ocr_region.y;

        printf("SUCCESS: Found and expanded layout text region: %dx%d at (%d,%d), confidence: %.2f\n",
               ocr_region.width, ocr_region.height, ocr_region.x, ocr_region.y,
               layout_region.confidence);
    }
    else
    {
        /* Если не нашли текст раскладки, используем весь регион трея,
           но увеличим его для лучшего OCR */
        int padding = 20;
        ocr_region.x = screen_region.x - padding;
        ocr_region.y = screen_region.y - padding;
        ocr_region.width = screen_region.width + 2 * padding;
        ocr_region.height = screen_region.height + 2 * padding;

        /* Убедимся, что не вышли за границы экрана */
        int screen_width = get_screen_width();
        int screen_height = get_screen_height();

        if (ocr_region.x < 0)
            ocr_region.x = 0;
        if (ocr_region.y < 0)
            ocr_region.y = 0;
        if (ocr_region.x + ocr_region.width > screen_width)
            ocr_region.width = screen_width - ocr_region.x;
        if (ocr_region.y + ocr_region.height > screen_height)
            ocr_region.height = screen_height - ocr_region.y;

        printf("WARNING: Using expanded tray region for OCR (text region not found): %dx%d at (%d,%d)\n",
               ocr_region.width, ocr_region.height, ocr_region.x, ocr_region.y);
    }

    /* Минимальный размер для OCR - увеличим если слишком маленький */
    if (ocr_region.width < 150)
    { /* Увеличьте с 80 до 150 */
        int diff = 150 - ocr_region.width;
        ocr_region.x -= diff / 2;
        ocr_region.width = 150;
    }
    if (ocr_region.height < 80)
    { /* Увеличьте с 40 до 80 */
        int diff = 80 - ocr_region.height;
        ocr_region.y -= diff / 2;
        ocr_region.height = 80;
    }

    /* Захватываем регион для OCR */
    char filename[256];
    sprintf(filename, "debug_layout_%lu.bmp", (unsigned long)GetCurrentProcessId());

    printf("Capturing screen region for OCR...\n");
    printf("OCR region: %dx%d at (%d,%d)\n", ocr_region.width, ocr_region.height, ocr_region.x, ocr_region.y);

    if (!capture_screen_region(ocr_region.x, ocr_region.y,
                               ocr_region.width, ocr_region.height,
                               filename))
    {
        printf("ERROR: Failed to capture layout region\n");
        opencv_cleanup();
        ocr_cleanup();
        return -1;
    }
    save_image_for_debug(ocr_region, "ocr_input");
    /* После сохранения debug изображения добавьте: */
    printf("\n=== DEBUG INFO ===\n");
    printf("Screen region: %dx%d at (%d,%d)\n",
           screen_region.width, screen_region.height, screen_region.x, screen_region.y);
    printf("Layout region found by OpenCV: %dx%d at (%d,%d), confidence: %.2f\n",
           layout_region.width, layout_region.height,
           layout_region.x, layout_region.y, layout_region.confidence);
    printf("Final OCR region: %dx%d at (%d,%d)\n",
           ocr_region.width, ocr_region.height, ocr_region.x, ocr_region.y);
    printf("Screen coordinates: absolute X=%d, Y=%d\n", ocr_region.x, ocr_region.y);
    printf("==================\n\n");

    printf("SUCCESS: Saved OCR image: %s\n", filename);

    /* Распознаем текст */
    printf("Running OCR with language: eng+rus...\n");
    OCRResult result = ocr_from_file(filename, OCR_LANG_ENGLISH_RUSSIAN, 7, 300);
    int layout = -2;

    /* Анализ результата OCR (оставьте без изменений)... */
    if (result.error_code == OCR_SUCCESS && result.text)
    {
        printf("OCR SUCCESS: Recognized text: '%s' (confidence: %.2f)\n",
               result.text, result.confidence);

        /* Простой и надежный анализ */
        char *text = result.text;
        int eng_score = 0;
        int rus_score = 0;

        /* Подсчитываем латинские и кириллические символы */
        for (int i = 0; text[i]; i++)
        {
            unsigned char c = (unsigned char)text[i];
            if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
            {
                eng_score++;
            }
            else if (c >= 0xC0 && c <= 0xFF)
            {
                rus_score++;
            }
        }

        /* Проверяем ключевые слова */
        if (strstr(text, "ENG") || strstr(text, "EN") || strstr(text, "US") ||
            strstr(text, "USA") || (eng_score > rus_score && eng_score >= 2))
        {
            printf("Detected: ENG (English)\n");
            layout = 0;
        }
        else if (strstr(text, "RUS") || strstr(text, "RU") ||
                 strstr(text, "РУС") || strstr(text, "РУ") ||
                 (rus_score > eng_score && rus_score >= 2))
        {
            printf("Detected: RUS (Russian)\n");
            layout = 1;
        }
        else
        {
            printf("WARNING: Cannot determine layout from text\n");
            printf("  English letters: %d, Russian letters: %d\n", eng_score, rus_score);
            layout = -2;
        }
    }
    else
    {
        printf("OCR FAILED with error code: %d\n", result.error_code);
        if (result.error_code == OCR_ERROR_NO_TEXT)
        {
            printf("No text found. Try increasing the capture area.\n");
        }
        else if (result.error_code == OCR_ERROR_TESSERACT_NOT_FOUND)
        {
            printf("Tesseract OCR not found. Please install from:\n");
            printf("https://github.com/UB-Mannheim/tesseract/wiki\n");
        }
        layout = -2;
    }

    /* Удаляем временный файл */
    printf("Cleaning up temporary files...\n");
    DeleteFile(filename);

    ocr_result_free(&result);
    opencv_cleanup();
    ocr_cleanup();

    printf("OpenCV+OCR method completed.\n");

    return layout;
}

/* Функция для тестирования OpenCV */
void test_opencv_detection(void)
{
    printf("\n=== Testing OpenCV System Tray Detection ===\n");

    if (!opencv_init())
    {
        printf("OpenCV initialization failed\n");
        return;
    }

    /* Получаем регион системного трея через OpenCV */
    DetectedRegion region = detect_system_tray_region_opencv(); /* ИЗМЕНЕНО */

    printf("OpenCV detection result: %dx%d at (%d,%d), confidence: %.2f\n",
           region.width, region.height, region.x, region.y, region.confidence);

    /* Захватываем и сохраняем изображение для проверки */
    if (region.width > 0 && region.height > 0)
    {
        char filename[256];
        sprintf(filename, "test_opencv_%lu.bmp", (unsigned long)GetCurrentProcessId());

        capture_screen_region(region.x, region.y, region.width, region.height, filename);
        printf("Test image saved: %s\n", filename);
    }

    /* Тестируем поиск текста раскладки */
    if (region.width > 0 && region.height > 0)
    {
        ScreenRegion screen_region;
        screen_region.x = region.x;
        screen_region.y = region.y;
        screen_region.width = region.width;
        screen_region.height = region.height;

        DetectedRegion layout_region = find_keyboard_layout_in_region(screen_region);

        if (layout_region.width > 0)
        {
            printf("Found layout text region: %dx%d at (%d,%d)\n",
                   layout_region.width, layout_region.height,
                   layout_region.x, layout_region.y);
        }
    }

    opencv_cleanup();
}

/* Main keyboard layout detection function */
int detect_keyboard_layout(int method)
{
    printf("=== Keyboard Layout Detection ===\n\n");

    int layout = -1;

    switch (method)
    {
    case 0: /* Windows API */
        printf("1. Windows API method:\n");
        layout = detect_layout_windows_api();
        break;

    case 1: /* OCR с OpenCV */
        printf("1. OCR with OpenCV method:\n");
        layout = detect_layout_ocr_opencv();
        break;

    case 2: /* Автоматический выбор */
    default:
        /* Сначала пробуем Windows API */
        printf("1. Trying Windows API method:\n");
        layout = detect_layout_windows_api();

        /* Если не сработало, используем OCR с OpenCV */
        if (layout < 0)
        {
            printf("\n2. Windows API failed, trying OCR with OpenCV:\n");
            layout = detect_layout_ocr_opencv();
        }
        break;
    }

    /* Вывод результата */
    if (layout >= 0)
    {
        if (layout == 0)
        {
            printf("   Result: ENG (English)\n\n");
        }
        else if (layout == 1)
        {
            printf("   Result: RUS (Russian)\n\n");
        }
        else
        {
            printf("   Result: UNKNOWN\n\n");
        }
    }
    else
    {
        printf("Failed to detect keyboard layout\n");
    }

    return layout;
}

void run_comprehensive_test(void)
{
    printf("\n=== COMPREHENSIVE TEST MODE ===\n");

    /* 1. Тест Windows API */
    printf("\n1. Testing Windows API method:\n");
    int api_result = detect_layout_windows_api();
    printf("Windows API result: %s\n",
           api_result == 0 ? "ENG" : api_result == 1 ? "RUS"
                                                     : "FAILED");

    /* 2. Тест OpenCV детекции трея */
    printf("\n2. Testing OpenCV tray detection:\n");
    if (!opencv_init())
    {
        printf("OpenCV initialization failed\n");
    }
    else
    {
        DetectedRegion tray = detect_system_tray_region_opencv(); /* ИЗМЕНЕНО */
        printf("Tray detection: %dx%d at (%d,%d), confidence: %.2f\n",
               tray.width, tray.height, tray.x, tray.y, tray.confidence);

        /* Сохраняем скриншот обнаруженного трея */
        if (tray.width > 0 && tray.height > 0)
        {
            char filename[256];
            sprintf(filename, "test_tray_detection.bmp");
            capture_screen_region(tray.x, tray.y, tray.width, tray.height, filename);
            printf("Saved tray screenshot: %s\n", filename);
        }

        opencv_cleanup();
    }

    /* 3. Тест OCR */
    printf("\n3. Testing OCR system:\n");
    if (ocr_init(NULL) != OCR_SUCCESS)
    {
        printf("OCR initialization failed\n");
    }
    else
    {
        /* Тестируем OCR на фиксированной области трея */
        ScreenRegion fixed = get_system_tray_region();
        char filename[256];
        sprintf(filename, "test_ocr_input.bmp");

        if (capture_screen_region(fixed.x, fixed.y, fixed.width, fixed.height, filename))
        {
            OCRResult ocr_result = ocr_from_file(filename, "eng+rus", 6, 300);
            printf("OCR test result: ");
            if (ocr_result.error_code == OCR_SUCCESS && ocr_result.text)
            {
                printf("text='%s', confidence=%.2f\n",
                       ocr_result.text, ocr_result.confidence);
            }
            else
            {
                printf("failed with error: %d\n", ocr_result.error_code);
            }
            ocr_result_free(&ocr_result);
        }

        ocr_cleanup();
    }

    /* 4. Полный тест OpenCV+OCR */
    printf("\n4. Testing complete OpenCV+OCR pipeline:\n");
    int ocr_result = detect_layout_ocr_opencv();
    printf("OpenCV+OCR result: %s\n",
           ocr_result == 0 ? "ENG" : ocr_result == 1 ? "RUS"
                                                     : "FAILED");

    printf("\n=== COMPREHENSIVE TEST COMPLETE ===\n");
}

/* Main function */
int main(int argc, char *argv[])
{
    printf("=========================================\n");
    printf("   Keyboard Layout Detector v3.0\n");
    printf("   (with simple computer vision)\n");
    printf("=========================================\n\n");

    int detection_method = 2; /* Автоматический выбор по умолчанию */
    int test_mode = 0;

    /* Обработка аргументов командной строки */
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-api") == 0)
        {
            detection_method = 0; /* Только Windows API */
        }
        else if (strcmp(argv[i], "-opencv") == 0)
        {
            detection_method = 1; /* Только OpenCV + OCR */
        }
        else if (strcmp(argv[i], "-test") == 0)
        {
            test_mode = 1;
        }
        else if (strcmp(argv[i], "-fulltest") == 0)
        {
            run_comprehensive_test();
            return 0;
        }
        else if (strcmp(argv[i], "-fixed") == 0)
        {
            /* Для обратной совместимости */
            detection_method = 0;
        }
    }

    /* Режим тестирования */
    if (test_mode)
    {
        test_opencv_detection();
        return 0;
    }

    /* Определение раскладки */
    int layout = detect_keyboard_layout(detection_method);

    printf("=========================================\n");
    printf("FINAL LAYOUT: %s\n", layout >= 0 ? (layout == 0 ? "ENG" : "RUS") : "UNKNOWN");
    printf("=========================================\n");

    /* Дополнительная информация */
    printf("\nAdditional information:\n");
    printf("- Screen resolution: %dx%d\n", get_screen_width(), get_screen_height());

    /* Сравнение методов */
    ScreenRegion fixed_region = get_system_tray_region();
    printf("- Fixed indicator region: %dx%d at (%d,%d)\n",
           fixed_region.width, fixed_region.height, fixed_region.x, fixed_region.y);

    /* Сохранение скриншотов для отладки */
    if (capture_screen_region(fixed_region.x, fixed_region.y,
                              fixed_region.width, fixed_region.height,
                              "debug_fixed.bmp"))
    {
        printf("- Fixed screenshot saved: debug_fixed.bmp\n");
    }

    printf("\nUsage:\n");
    printf("  No parameters - automatic detection (recommended)\n");
    printf("  -api          - use Windows API only\n");
    printf("  -opencv       - use OpenCV + OCR only\n");
    printf("  -test         - test OpenCV detection\n");
    printf("  -fixed        - use fixed region (legacy mode)\n");

    return 0;
}
