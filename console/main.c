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

/* Функция для OCR из региона с возвратом текста */
OCRResult ocr_from_region(ScreenRegion region, const char *language, int psm, int dpi)
{
    char filename[256];
    sprintf(filename, "temp_ocr_region_%lu.bmp", (unsigned long)GetCurrentProcessId());

    if (!capture_screen_region(region.x, region.y, region.width, region.height, filename))
    {
        OCRResult error_result = {0};
        error_result.error_code = OCR_ERROR_CAPTURE_FAILED;
        return error_result;
    }

    OCRResult result = ocr_from_file(filename, language, psm, dpi);
    DeleteFile(filename);
    return result;
}

/* Функция для очистки и форматирования текста OCR */
char *clean_ocr_text_for_display(const char *text)
{
    if (!text || strlen(text) == 0)
    {
        char *empty = (char *)malloc(2);
        empty[0] = '-';
        empty[1] = '\0';
        return empty;
    }

    /* Выделяем память для очищенного текста */
    char *cleaned = (char *)malloc(strlen(text) + 1);
    int j = 0;

    /* Удаляем лишние пробелы и непечатные символы */
    for (int i = 0; text[i]; i++)
    {
        unsigned char c = (unsigned char)text[i];
        if (c >= 32 && c <= 126) /* Печатные ASCII символы */
        {
            cleaned[j++] = c;
        }
        else if (c >= 0xC0 && c <= 0xFF) /* Кириллица в CP1251 */
        {
            cleaned[j++] = c;
        }
        else if (c == '\n' || c == '\r')
        {
            if (j > 0 && cleaned[j - 1] != ' ')
                cleaned[j++] = ' ';
        }
    }

    cleaned[j] = '\0';

    /* Удаляем начальные и конечные пробелы */
    while (j > 0 && cleaned[0] == ' ')
    {
        memmove(cleaned, cleaned + 1, j);
        j--;
    }

    while (j > 0 && cleaned[j - 1] == ' ')
    {
        cleaned[j - 1] = '\0';
        j--;
    }

    /* Если текст пустой, возвращаем "-" */
    if (j == 0)
    {
        free(cleaned);
        char *dash = (char *)malloc(2);
        dash[0] = '-';
        dash[1] = '\0';
        return dash;
    }

    return cleaned;
}

/* Основная функция: разделяет область на 8 регионов и распознает текст */
void analyze_8grid_regions(ScreenRegion tray_region)
{
    printf("\n=== 8-REGION GRID ANALYSIS (4x2) ===\n");
    printf("Full tray region: %dx%d at (%d,%d)\n",
           tray_region.width, tray_region.height, tray_region.x, tray_region.y);

    if (ocr_init(NULL) != OCR_SUCCESS)
    {
        printf("OCR initialization failed\n");
        return;
    }

    /* Разделяем на 4 столбца и 2 строки */
    int grid_cols = 4;
    int grid_rows = 2;
    int cell_width = tray_region.width / grid_cols;
    int cell_height = tray_region.height / grid_rows;

    printf("Cell size: %dx%d\n", cell_width, cell_height);

    /* Массивы для хранения распознанного текста */
    char *row1_texts[4] = {NULL, NULL, NULL, NULL};
    char *row2_texts[4] = {NULL, NULL, NULL, NULL};

    /* Распознаем текст в каждом регионе */
    for (int row = 0; row < grid_rows; row++)
    {
        for (int col = 0; col < grid_cols; col++)
        {
            /* Вычисляем координаты региона */
            ScreenRegion cell;
            cell.x = tray_region.x + col * cell_width;
            cell.y = tray_region.y + row * cell_height;
            cell.width = cell_width;
            cell.height = cell_height;

            /* Небольшая коррекция для избежания перекрытия */
            if (col < grid_cols - 1)
                cell.width -= 1;
            if (row < grid_rows - 1)
                cell.height -= 1;

            printf("\n--- Region [%d,%d]: %dx%d at (%d,%d) ---\n",
                   row, col, cell.width, cell.height, cell.x, cell.y);

            /* Сохраняем изображение региона для отладки */
            char debug_name[50];
            sprintf(debug_name, "region_%d_%d", row, col);
            save_image_for_debug(cell, debug_name);

            /* Пробуем несколько стратегий OCR для лучшего результата */
            char *best_text = NULL;
            float best_confidence = 0;

            for (int strategy = 0; strategy < 3; strategy++)
            {
                const char *lang = (strategy == 0) ? OCR_LANG_ENGLISH : (strategy == 1) ? OCR_LANG_RUSSIAN
                                                                                        : OCR_LANG_ENGLISH_RUSSIAN;
                int psm = (strategy == 0) ? 8 : (strategy == 1) ? 8
                                                                : 7;

                OCRResult result = ocr_from_region(cell, lang, psm, 300);

                if (result.error_code == OCR_SUCCESS && result.text && strlen(result.text) > 0)
                {
                    /* Очищаем текст для отображения */
                    char *cleaned_text = clean_ocr_text_for_display(result.text);

                    if (strlen(cleaned_text) > 0 && result.confidence > best_confidence)
                    {
                        if (best_text)
                            free(best_text);
                        best_text = cleaned_text;
                        best_confidence = result.confidence;

                        printf("  Strategy %d: '%s' (confidence: %.2f)\n",
                               strategy + 1, cleaned_text, result.confidence);
                    }
                    else
                    {
                        free(cleaned_text);
                    }
                }

                ocr_result_free(&result);
            }

            /* Сохраняем лучший текст */
            if (best_text && strlen(best_text) > 0)
            {
                if (row == 0)
                    row1_texts[col] = best_text;
                else
                    row2_texts[col] = best_text;
            }
            else
            {
                char *dash = (char *)malloc(2);
                dash[0] = '-';
                dash[1] = '\0';
                if (row == 0)
                    row1_texts[col] = dash;
                else
                    row2_texts[col] = dash;
            }
        }
    }

    /* Формируем и выводим 2 текстовые строки */
    printf("\n=== RECOGNITION RESULTS ===\n");

    /* Первая строка (верхний ряд) */
    printf("\nROW 1 (top): ");
    for (int col = 0; col < grid_cols; col++)
    {
        if (row1_texts[col])
        {
            printf("[%d]='%s' ", col + 1, row1_texts[col]);
        }
        else
        {
            printf("[%d]='-' ", col + 1);
        }
    }

    /* Вторая строка (нижний ряд) */
    printf("\nROW 2 (bottom): ");
    for (int col = 0; col < grid_cols; col++)
    {
        if (row2_texts[col])
        {
            printf("[%d]='%s' ", col + 1, row2_texts[col]);
        }
        else
        {
            printf("[%d]='-' ", col + 1);
        }
    }

    /* Определяем раскладку клавиатуры на основе распознанного текста */
    printf("\n\n=== LAYOUT DETECTION ===\n");

    int eng_score = 0;
    int rus_score = 0;

    /* Анализируем все распознанные тексты */
    for (int col = 0; col < grid_cols; col++)
    {
        if (row1_texts[col])
        {
            char *text = row1_texts[col];
            if (strstr(text, "ENG") || strstr(text, "EN") || strstr(text, "US"))
                eng_score++;
            else if (strstr(text, "RUS") || strstr(text, "RU") ||
                     strstr(text, "РУС") || strstr(text, "РУ"))
                rus_score++;
        }

        if (row2_texts[col])
        {
            char *text = row2_texts[col];
            if (strstr(text, "ENG") || strstr(text, "EN") || strstr(text, "US"))
                eng_score++;
            else if (strstr(text, "RUS") || strstr(text, "RU") ||
                     strstr(text, "РУС") || strstr(text, "РУ"))
                rus_score++;
        }
    }

    printf("ENG indicators found: %d\n", eng_score);
    printf("RUS indicators found: %d\n", rus_score);

    if (eng_score > rus_score)
    {
        printf("DETECTED LAYOUT: ENG (English)\n");
    }
    else if (rus_score > eng_score)
    {
        printf("DETECTED LAYOUT: RUS (Russian)\n");
    }
    else if (eng_score == rus_score && eng_score > 0)
    {
        printf("AMBIGUOUS: Both ENG and RUS indicators found\n");
    }
    else
    {
        printf("NO LAYOUT INDICATORS FOUND\n");
    }

    /* Освобождаем память */
    for (int col = 0; col < grid_cols; col++)
    {
        if (row1_texts[col])
            free(row1_texts[col]);
        if (row2_texts[col])
            free(row2_texts[col]);
    }

    ocr_cleanup();
    printf("\n=== 8-REGION ANALYSIS COMPLETE ===\n");
}

/* Улучшенная версия detect_layout_ocr_opencv с анализом 8 регионов */
int detect_layout_ocr_opencv_8grid(void)
{
    printf("\n=== OCR with OpenCV - 8 Region Analysis ===\n");

    /* Инициализируем OpenCV */
    printf("Initializing OpenCV...\n");
    if (!opencv_init())
    {
        printf("OpenCV initialization failed\n");
        return -1;
    }

    /* Получаем регион системного трея через OpenCV */
    printf("Detecting system tray region...\n");
    DetectedRegion tray_region = detect_system_tray_region_opencv();

    if (tray_region.width == 0 || tray_region.height == 0)
    {
        printf("WARNING: OpenCV failed to detect system tray\n");
        printf("Using fixed region...\n");
        opencv_cleanup();

        /* Используем фиксированную область */
        ScreenRegion fixed_region = get_system_tray_region();

        /* Анализируем 8 регионов и определяем раскладку */
        analyze_8grid_regions(fixed_region);
        return -1; /* Раскладка уже выведена в analyze_8grid_regions */
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

    /* Сохраняем полный скриншот трея для отладки */
    save_image_for_debug(screen_region, "full_tray_opencv");

    /* Анализируем 8 регионов и определяем раскладку */
    analyze_8grid_regions(screen_region);

    opencv_cleanup();
    return -1; /* Раскладка уже выведена в analyze_8grid_regions */
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
    }

    opencv_cleanup();
}

/* Функция для тестирования разных областей захвата */
void test_different_regions(void)
{
    printf("\n=== TESTING DIFFERENT CAPTURE REGIONS ===\n");

    /* Получаем различные регионы */
    ScreenRegion fixed_region = get_system_tray_region();
    printf("1. Fixed region: %dx%d at (%d,%d)\n",
           fixed_region.width, fixed_region.height,
           fixed_region.x, fixed_region.y);

    if (!opencv_init())
    {
        printf("OpenCV initialization failed\n");
        return;
    }

    DetectedRegion opencv_tray = detect_system_tray_region_opencv();
    printf("2. OpenCV tray region: %dx%d at (%d,%d)\n",
           opencv_tray.width, opencv_tray.height,
           opencv_tray.x, opencv_tray.y);

    /* Тестируем несколько областей */
    ScreenRegion test_regions[] = {
        /* Область 1: Фиксированная область */
        {fixed_region.x, fixed_region.y, fixed_region.width, fixed_region.height},

        /* Область 2: Смещенная область */
        {1640, 1010, 120, 50},

        /* Область 3: Правая часть OpenCV трея */
        {opencv_tray.x + opencv_tray.width * 0.7,
         opencv_tray.y + opencv_tray.height * 0.3,
         100, 40},

        /* Область 4: Левая часть OpenCV трея */
        {opencv_tray.x + 20,
         opencv_tray.y + opencv_tray.height * 0.3,
         100, 40},
    };

    const char *region_names[] = {
        "Fixed region",
        "Current region (1640,1010)",
        "Right side of OpenCV tray",
        "Left side of OpenCV tray"};

    for (int i = 0; i < 4; i++)
    {
        printf("\n--- Testing region %d: %s ---\n", i + 1, region_names[i]);
        printf("Region: %dx%d at (%d,%d)\n",
               test_regions[i].width, test_regions[i].height,
               test_regions[i].x, test_regions[i].y);

        /* Сохраняем изображение */
        char filename[256];
        sprintf(filename, "test_region_%d_%lu.bmp", i + 1, (unsigned long)GetCurrentProcessId());
        capture_screen_region(test_regions[i].x, test_regions[i].y,
                              test_regions[i].width, test_regions[i].height,
                              filename);
        printf("Saved: %s\n", filename);

        /* Анализируем 8 регионов (для больших областей) */
        if (test_regions[i].width >= 100 && test_regions[i].height >= 50)
        {
            analyze_8grid_regions(test_regions[i]);
        }
    }

    opencv_cleanup();
    printf("\n=== REGION TEST COMPLETE ===\n");
}

/* Основная функция определения раскладки */
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

    case 1: /* OCR с OpenCV (анализ 8 регионов) */
        printf("1. OCR with OpenCV (8-region analysis):\n");
        detect_layout_ocr_opencv_8grid();
        layout = -1; /* Раскладка уже выведена внутри функции */
        break;

    case 2: /* Автоматический выбор */
    default:
        /* Сначала пробуем Windows API */
        printf("1. Trying Windows API method:\n");
        layout = detect_layout_windows_api();

        /* Если не сработало, используем анализ 8 регионов с OpenCV */
        if (layout < 0)
        {
            printf("\n2. Windows API failed, trying 8-region OCR with OpenCV:\n");
            detect_layout_ocr_opencv_8grid();
            layout = -1;
        }
        break;
    }

    /* Вывод результата (если еще не выведен в analyze_8grid_regions) */
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

    /* 3. Тест анализа 8 регионов на фиксированной области */
    printf("\n3. Testing 8-region analysis on fixed region:\n");
    ScreenRegion fixed = get_system_tray_region();
    printf("Fixed region: %dx%d at (%d,%d)\n",
           fixed.width, fixed.height, fixed.x, fixed.y);

    analyze_8grid_regions(fixed);

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
        printf("Windows API failed, trying 8-region analysis...\n");
        ScreenRegion fixed = get_system_tray_region();
        analyze_8grid_regions(fixed);
    }
}

/* Main function */
int main(int argc, char *argv[])
{
    printf("=========================================\n");
    printf("   Keyboard Layout Detector v4.0\n");
    printf("   (8-region grid analysis)\n");
    printf("=========================================\n\n");

    int detection_method = 2; /* Автоматический выбор по умолчанию */
    int test_mode = 0;
    int quick_mode = 0;
    int grid_mode = 0;

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
        else if (strcmp(argv[i], "-grid") == 0)
        {
            grid_mode = 1; /* Режим 8-регионов */
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
        else if (strcmp(argv[i], "-testregions") == 0)
        {
            test_different_regions();
            return 0;
        }
        else if (strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            printf("\nUsage:\n");
            printf("  automator_console.exe [options]\n\n");
            printf("Options:\n");
            printf("  -api         - use Windows API only (fastest)\n");
            printf("  -opencv      - use OpenCV + 8-region analysis\n");
            printf("  -grid        - 8-region grid analysis mode\n");
            printf("  -fixed       - use fixed region (legacy)\n");
            printf("  -test        - test OpenCV detection\n");
            printf("  -testregions - test different capture regions\n");
            printf("  -fulltest    - run comprehensive tests\n");
            printf("  -quick       - quick layout check\n");
            printf("  -help        - show this help\n\n");
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

    /* Режим 8-регионов */
    if (grid_mode)
    {
        ScreenRegion fixed_region = get_system_tray_region();
        analyze_8grid_regions(fixed_region);
        return 0;
    }

    /* Определение раскладки */
    int layout = detect_keyboard_layout(detection_method);

    printf("=========================================\n");
    if (layout >= 0)
    {
        printf("FINAL LAYOUT: %s\n", layout == 0 ? "ENG" : "RUS");
    }
    else
    {
        printf("LAYOUT ANALYSIS COMPLETE\n");
    }
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
    printf("  -opencv       - use OpenCV + 8-region analysis\n");
    printf("  -grid         - 8-region grid analysis mode\n");
    printf("  -test         - test OpenCV detection\n");
    printf("  -testregions  - test different capture regions\n");
    printf("  -fulltest     - run comprehensive tests\n");
    printf("  -quick        - quick layout check\n");
    printf("  -fixed        - use fixed region (legacy mode)\n");
    printf("  -help         - show help\n");

    return 0;
}