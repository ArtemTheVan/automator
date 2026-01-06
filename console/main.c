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

/* Функция для определения раскладки из фиксированной области */
int detect_layout_from_region(ScreenRegion region)
{
    char filename[256];
    sprintf(filename, "temp_layout_%lu.bmp", (unsigned long)GetCurrentProcessId());

    printf("Capturing fixed region: %dx%d at (%d,%d)\n",
           region.width, region.height, region.x, region.y);

    if (!capture_screen_region(region.x, region.y, region.width, region.height, filename))
    {
        printf("ERROR: Failed to capture layout region\n");
        return -1;
    }

    OCRResult result = ocr_from_file(filename, OCR_LANG_ENGLISH_RUSSIAN, 6, 300);
    int layout = -2;

    if (result.error_code == OCR_SUCCESS && result.text)
    {
        printf("OCR SUCCESS: Recognized text: '%s' (confidence: %.2f)\n",
               result.text, result.confidence);

        char *text = result.text;
        int eng_score = 0;
        int rus_score = 0;

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
        layout = -2;
    }

    DeleteFile(filename);
    ocr_result_free(&result);

    return layout;
}

/* Улучшенная функция для определения области индикатора раскладки */
ScreenRegion get_layout_indicator_region(ScreenRegion tray_region)
{
    ScreenRegion layout_region;
    
    /* Стратегия 1: Пробуем разные позиции в трее */

    layout_region.x = tray_region.x;
    layout_region.y = tray_region.y;
    layout_region.width = tray_region.width;
    layout_region.height = tray_region.height;

    /* Вариант A: Левая часть трея (наиболее вероятное место для индикатора) */
    // layout_region.x = tray_region.x + 10;  /* Отступ от левого края */
    // layout_region.y = tray_region.y + (tray_region.height - 40) / 2; /* Центр по вертикали */
    // layout_region.width = 160;  /* Ширина достаточная для "ENG" или "RUS" */
    // layout_region.height = 60;  /* Высота для текста */
    
    /* Вариант B: Центральная часть трея */
    /* layout_region.x = tray_region.x + tray_region.width / 3; */
    /* layout_region.y = tray_region.y + 5; */
    /* layout_region.width = 80; */
    /* layout_region.height = 35; */
    
    /* Вариант C: Фиксированная позиция (как в get_system_tray_region()) */
    /* layout_region = get_system_tray_region(); */
    
    printf("Using layout region: %dx%d at (%d,%d)\n",
           layout_region.width, layout_region.height,
           layout_region.x, layout_region.y);
    
    return layout_region;
}

/* Улучшенная версия detect_layout_ocr_opencv с несколькими стратегиями */
int detect_layout_ocr_opencv_improved(void)
{
    printf("\n=== IMPROVED OCR with OpenCV Method ===\n");

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

    /* Получаем регион системного трея через OpenCV */
    printf("Detecting system tray region...\n");
    DetectedRegion tray_region = detect_system_tray_region_opencv();

    if (tray_region.width == 0 || tray_region.height == 0)
    {
        printf("WARNING: OpenCV failed to detect system tray\n");
        printf("Using fixed region as fallback...\n");
        opencv_cleanup();
        
        /* Используем фиксированную область как fallback */
        ScreenRegion fixed_region = get_system_tray_region();
        int layout = detect_layout_from_region(fixed_region);
        ocr_cleanup();
        return layout;
    }

    printf("SUCCESS: OpenCV detected tray region: %dx%d at (%d,%d), confidence: %.2f\n",
           tray_region.width, tray_region.height, tray_region.x, tray_region.y,
           tray_region.confidence);

    /* Конвертируем в ScreenRegion */
    ScreenRegion screen_region;
    screen_region.x = tray_region.x;
    screen_region.y = tray_region.y;
    screen_region.width = tray_region.width;
    screen_region.height = tray_region.height;

    /* Стратегия 1: Используем улучшенный алгоритм поиска области индикатора */
    ScreenRegion layout_region = get_layout_indicator_region(screen_region);
    
    /* Сохраняем debug изображение области поиска */
    save_image_for_debug(layout_region, "layout_region");

    /* Пробуем несколько OCR стратегий */
    int layout = -2;
    int strategies_tried = 0;
    
    /* Стратегия 1: Прямой захват области индикатора */
    printf("\n--- Strategy 1: Direct layout region capture ---\n");
    layout = detect_layout_from_region(layout_region);
    strategies_tried++;
    
    /* Стратегия 2: Если первая не сработала, пробуем увеличенную область */
    if (layout < 0)
    {
        printf("\n--- Strategy 2: Expanded region capture ---\n");
        ScreenRegion expanded_region = layout_region;
        expanded_region.x -= 20;
        expanded_region.y -= 10;
        expanded_region.width += 40;
        expanded_region.height += 20;
        
        /* Проверяем границы экрана */
        int screen_width = get_screen_width();
        int screen_height = get_screen_height();
        
        if (expanded_region.x < 0) expanded_region.x = 0;
        if (expanded_region.y < 0) expanded_region.y = 0;
        if (expanded_region.x + expanded_region.width > screen_width)
            expanded_region.width = screen_width - expanded_region.x;
        if (expanded_region.y + expanded_region.height > screen_height)
            expanded_region.height = screen_height - expanded_region.y;
        
        layout = detect_layout_from_region(expanded_region);
        strategies_tried++;
    }
    
    /* Стратегия 3: Используем весь трей (последняя попытка) */
    if (layout < 0)
    {
        printf("\n--- Strategy 3: Full tray region capture ---\n");
        layout = detect_layout_from_region(screen_region);
        strategies_tried++;
    }
    
    /* Стратегия 4: Используем фиксированную область (аварийный fallback) */
    if (layout < 0)
    {
        printf("\n--- Strategy 4: Fixed region fallback ---\n");
        ScreenRegion fixed_region = get_system_tray_region();
        layout = detect_layout_from_region(fixed_region);
        strategies_tried++;
    }
    
    printf("\nTried %d OCR strategies. ", strategies_tried);
    if (layout >= 0)
    {
        printf("Success with layout: %s\n", layout == 0 ? "ENG" : "RUS");
    }
    else
    {
        printf("All strategies failed.\n");
    }

    opencv_cleanup();
    ocr_cleanup();

    printf("Improved OpenCV+OCR method completed.\n");

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
    DetectedRegion region = detect_system_tray_region_opencv();

    printf("OpenCV detection result: %dx%d at (%d,%d), confidence: %.2f\n",
           region.width, region.height, region.x, region.y, region.confidence);

    /* Захватываем и сохраняем изображение для проверки */
    if (region.width > 0 && region.height > 0)
    {
        char filename[256];
        sprintf(filename, "test_opencv_%lu.bmp", (unsigned long)GetCurrentProcessId());

        capture_screen_region(region.x, region.y, region.width, region.height, filename);
        printf("Test image saved: %s\n", filename);
        
        /* Также сохраняем предлагаемую область для индикатора */
        ScreenRegion screen_region;
        screen_region.x = region.x;
        screen_region.y = region.y;
        screen_region.width = region.width;
        screen_region.height = region.height;
        
        ScreenRegion layout_region = get_layout_indicator_region(screen_region);
        char layout_filename[256];
        sprintf(layout_filename, "test_layout_region_%lu.bmp", (unsigned long)GetCurrentProcessId());
        capture_screen_region(layout_region.x, layout_region.y, 
                             layout_region.width, layout_region.height, 
                             layout_filename);
        printf("Layout region image saved: %s\n", layout_filename);
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

    case 1: /* OCR с OpenCV (улучшенная версия) */
        printf("1. Improved OCR with OpenCV method:\n");
        layout = detect_layout_ocr_opencv_improved();
        break;

    case 2: /* Автоматический выбор */
    default:
        /* Сначала пробуем Windows API */
        printf("1. Trying Windows API method:\n");
        layout = detect_layout_windows_api();

        /* Если не сработало, используем улучшенный OCR с OpenCV */
        if (layout < 0)
        {
            printf("\n2. Windows API failed, trying improved OCR with OpenCV:\n");
            layout = detect_layout_ocr_opencv_improved();
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
        DetectedRegion tray = detect_system_tray_region_opencv();
        printf("Tray detection: %dx%d at (%d,%d), confidence: %.2f\n",
               tray.width, tray.height, tray.x, tray.y, tray.confidence);

        /* Сохраняем скриншот обнаруженного трея */
        if (tray.width > 0 && tray.height > 0)
        {
            char filename[256];
            sprintf(filename, "test_tray_detection_%lu.bmp", (unsigned long)GetCurrentProcessId());
            capture_screen_region(tray.x, tray.y, tray.width, tray.height, filename);
            printf("Saved tray screenshot: %s\n", filename);
        }

        opencv_cleanup();
    }

    /* 3. Тест OCR на фиксированной области */
    printf("\n3. Testing OCR on fixed region:\n");
    if (ocr_init(NULL) != OCR_SUCCESS)
    {
        printf("OCR initialization failed\n");
    }
    else
    {
        ScreenRegion fixed = get_system_tray_region();
        printf("Fixed region: %dx%d at (%d,%d)\n",
               fixed.width, fixed.height, fixed.x, fixed.y);
        
        int fixed_result = detect_layout_from_region(fixed);
        printf("Fixed region OCR result: %s\n",
               fixed_result == 0 ? "ENG" : fixed_result == 1 ? "RUS" : "FAILED");
        
        ocr_cleanup();
    }

    /* 4. Полный тест улучшенного OpenCV+OCR */
    printf("\n4. Testing improved OpenCV+OCR pipeline:\n");
    int ocr_result = detect_layout_ocr_opencv_improved();
    printf("Improved OpenCV+OCR result: %s\n",
           ocr_result == 0 ? "ENG" : ocr_result == 1 ? "RUS"
                                                     : "FAILED");

    printf("\n=== COMPREHENSIVE TEST COMPLETE ===\n");
}

/* Функция для быстрой проверки текущей раскладки */
void quick_check(void)
{
    printf("\n=== QUICK LAYOUT CHECK ===\n");
    
    /* Пробуем Windows API (самый быстрый метод) */
    int layout = detect_layout_windows_api();
    
    if (layout >= 0)
    {
        printf("Current layout: %s\n", layout == 0 ? "ENG" : "RUS");
    }
    else
    {
        /* Если Windows API не сработал, пробуем фиксированную область */
        printf("Windows API failed, trying OCR on fixed region...\n");
        
        if (ocr_init(NULL) == OCR_SUCCESS)
        {
            ScreenRegion fixed = get_system_tray_region();
            layout = detect_layout_from_region(fixed);
            ocr_cleanup();
            
            if (layout >= 0)
            {
                printf("Current layout: %s\n", layout == 0 ? "ENG" : "RUS");
            }
            else
            {
                printf("Could not determine keyboard layout\n");
            }
        }
        else
        {
            printf("OCR initialization failed\n");
        }
    }
}

/* Main function */
int main(int argc, char *argv[])
{
    printf("=========================================\n");
    printf("   Keyboard Layout Detector v3.1\n");
    printf("   (improved OpenCV + OCR)\n");
    printf("=========================================\n\n");

    int detection_method = 2; /* Автоматический выбор по умолчанию */
    int test_mode = 0;
    int quick_mode = 0;

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
            detection_method = 0; /* Для обратной совместимости */
        }
        else if (strcmp(argv[i], "-quick") == 0)
        {
            quick_mode = 1;
        }
        else if (strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            printf("\nUsage:\n");
            printf("  automator_console.exe [options]\n\n");
            printf("Options:\n");
            printf("  -api      - use Windows API only (fastest)\n");
            printf("  -opencv   - use improved OpenCV + OCR\n");
            printf("  -fixed    - use fixed region (legacy)\n");
            printf("  -test     - test OpenCV detection\n");
            printf("  -fulltest - run comprehensive tests\n");
            printf("  -quick    - quick layout check\n");
            printf("  -help     - show this help\n\n");
            return 0;
        }
    }

    /* Быстрая проверка */
    if (quick_mode)
    {
        quick_check();
        return 0;
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
    printf("  -opencv       - use improved OpenCV + OCR\n");
    printf("  -test         - test OpenCV detection\n");
    printf("  -fulltest     - run comprehensive tests\n");
    printf("  -quick        - quick layout check\n");
    printf("  -fixed        - use fixed region (legacy mode)\n");
    printf("  -help         - show help\n");

    return 0;
}
