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

/* Основная функция: распознавание текста в системном трее с помощью OpenCV */
void recognize_all_text_in_tray(void)
{
    printf("\n=== RECOGNIZING ALL TEXT IN SYSTEM TRAY ===\n");

    /* Инициализируем OpenCV */
    printf("Initializing OpenCV...\n");
    if (!opencv_init())
    {
        printf("OpenCV initialization failed\n");
        return;
    }

    /* Получаем регион системного трея через OpenCV */
    DetectedRegion tray_detected = detect_system_tray_region_opencv();

    if (tray_detected.width == 0 || tray_detected.height == 0)
    {
        printf("Failed to detect system tray. Using fixed region.\n");
        ScreenRegion fixed_region = get_system_tray_region();
        tray_detected.x = fixed_region.x;
        tray_detected.y = fixed_region.y;
        tray_detected.width = fixed_region.width;
        tray_detected.height = fixed_region.height;
        tray_detected.confidence = 0.5f;
    }

    ScreenRegion tray_region = {
        tray_detected.x,
        tray_detected.y,
        tray_detected.width,
        tray_detected.height};

    printf("System tray region: %dx%d at (%d,%d), confidence: %.2f\n",
           tray_region.width, tray_region.height,
           tray_region.x, tray_region.y, tray_detected.confidence);

    /* Сохраняем скриншот для отладки */
    save_image_for_debug(tray_region, "full_tray");

    /* Ищем текстовые регионы в трее с помощью OpenCV */
    int region_count = 0;
    DetectedRegion *text_regions = find_text_regions_in_tray(tray_region, &region_count);

    if (region_count == 0 || !text_regions)
    {
        printf("No text regions found. Using whole tray area.\n");
        text_regions = (DetectedRegion *)malloc(sizeof(DetectedRegion));
        text_regions[0] = tray_detected;
        region_count = 1;
    }

    /* Инициализируем OCR */
    if (ocr_init(NULL) != OCR_SUCCESS)
    {
        printf("OCR initialization failed\n");
        free(text_regions);
        opencv_cleanup();
        return;
    }

    printf("\n=== OCR RESULTS ===\n");

    /* Распознаем текст в каждом текстовом регионе */
    for (int i = 0; i < region_count; i++)
    {
        DetectedRegion region = text_regions[i];

        printf("\n--- Text Region %d: %dx%d at (%d,%d), confidence: %.2f ---\n",
               i + 1, region.width, region.height, region.x, region.y, region.confidence);

        /* Сохраняем регион для отладки */
        char debug_name[50];
        sprintf(debug_name, "text_region_%d", i);
        save_image_for_debug((ScreenRegion){region.x, region.y, region.width, region.height}, debug_name);

        /* Пробуем разные языки OCR */
        char *best_text = NULL;
        float best_confidence = 0;

        for (int strategy = 0; strategy < 3; strategy++)
        {
            const char *lang = (strategy == 0) ? OCR_LANG_ENGLISH : (strategy == 1) ? OCR_LANG_RUSSIAN
                                                                                    : OCR_LANG_ENGLISH_RUSSIAN;

            OCRResult result = ocr_from_region(
                (ScreenRegion){region.x, region.y, region.width, region.height},
                lang, 7, 300);

            if (result.error_code == OCR_SUCCESS && result.text && strlen(result.text) > 0)
            {
                char *cleaned_text = clean_ocr_text_for_display(result.text);

                /* Проверяем, что текст имеет смысл */
                int has_meaningful_chars = 0;
                for (int j = 0; cleaned_text[j]; j++)
                {
                    unsigned char c = cleaned_text[j];
                    if ((c >= 'A' && c <= 'Z') ||
                        (c >= 'a' && c <= 'z') ||
                        (c >= '0' && c <= '9') ||
                        (c >= 0xC0 && c <= 0xFF)) /* Кириллица */
                    {
                        has_meaningful_chars = 1;
                        break;
                    }
                }

                if (has_meaningful_chars && result.confidence > best_confidence)
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

        /* Выводим лучший результат */
        if (best_text && strlen(best_text) > 0)
        {
            printf("  Result: '%s'\n", best_text);
            free(best_text);
        }
        else
        {
            printf("  Result: No text recognized\n");
        }
    }

    free(text_regions);
    ocr_cleanup();
    opencv_cleanup();
    printf("\n=== TEXT RECOGNITION COMPLETE ===\n");
}

/* Основная функция */
int main(int argc, char *argv[])
{
    printf("=========================================\n");
    printf("   System Tray Text Recognizer v2.0\n");
    printf("   (OpenCV Text Detection + OCR)\n");
    printf("=========================================\n\n");

    /* Проверяем аргументы */
    int layout_only = 0;
    int help = 0;
    int test_opencv = 0;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-layout") == 0 || strcmp(argv[i], "-l") == 0)
            layout_only = 1;
        else if (strcmp(argv[i], "-test") == 0)
            test_opencv = 1;
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
            help = 1;
    }

    if (help)
    {
        printf("Usage:\n");
        printf("  automator_console.exe           - Recognize all text in system tray\n");
        printf("  automator_console.exe -layout   - Detect keyboard layout only\n");
        printf("  automator_console.exe -test     - Test OpenCV text detection\n");
        printf("  automator_console.exe -h        - Show this help\n");
        return 0;
    }

    if (layout_only)
    {
        /* Только определение раскладки */
        printf("\n=== KEYBOARD LAYOUT DETECTION ===\n");
        int layout = detect_layout_windows_api();

        if (layout == 0)
            printf("Detected layout: ENG (English)\n");
        else if (layout == 1)
            printf("Detected layout: RUS (Russian)\n");
        else
            printf("Could not detect layout\n");
    }
    else if (test_opencv)
    {
        /* Тест OpenCV детекции текста */
        printf("\n=== OPENCV TEXT DETECTION TEST ===\n");

        if (!opencv_init())
        {
            printf("OpenCV initialization failed\n");
            return 1;
        }

        /* Получаем регион трея */
        DetectedRegion tray = detect_system_tray_region_opencv();
        ScreenRegion region = {tray.x, tray.y, tray.width, tray.height};

        /* Ищем текстовые регионы */
        int region_count = 0;
        DetectedRegion *text_regions = find_text_regions_in_tray(region, &region_count);

        printf("\nFound %d text regions:\n", region_count);
        for (int i = 0; i < region_count; i++)
        {
            printf("  Region %d: %dx%d at (%d,%d), confidence: %.2f\n",
                   i + 1, text_regions[i].width, text_regions[i].height,
                   text_regions[i].x, text_regions[i].y, text_regions[i].confidence);

            /* Сохраняем каждый регион для визуальной проверки */
            char filename[256];
            sprintf(filename, "opencv_test_region_%d_%lu.bmp", i, (unsigned long)GetCurrentProcessId());
            capture_screen_region(text_regions[i].x, text_regions[i].y,
                                  text_regions[i].width, text_regions[i].height,
                                  filename);
            printf("    Saved to: %s\n", filename);
        }

        if (text_regions)
            free(text_regions);
        opencv_cleanup();
    }
    else
    {
        /* Полное распознавание текста с помощью OpenCV */
        recognize_all_text_in_tray();
    }

    printf("\n=========================================\n");
    printf("Program completed\n");
    printf("=========================================\n");

    return 0;
}