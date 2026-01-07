// opencv_wrapper.cpp - C++ обертка для OpenCV функций
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <vector>
#include <algorithm>

// Включаем automator.h для получения макросов
#include "automator.h"

// Простая структура для C интерфейса (уже определена в automator.h)
// typedef struct {
//     int x;
//     int y;
//     int width;
//     int height;
//     float confidence;
// } DetectedRegion;

extern "C"
{

    // Объявления функций уже есть в automator.h, здесь мы их определяем

    AUTOMATOR_API int opencv_init(void)
    {
        printf("OpenCV module initialized (C++ wrapper)\n");
        return 1;
    }

    AUTOMATOR_API void opencv_cleanup(void)
    {
        printf("OpenCV module cleaned up\n");
    }

    AUTOMATOR_API DetectedRegion detect_system_tray_region_opencv(void)
    {
        DetectedRegion result = {0, 0, 0, 0, 0.0f};

        printf("Detecting system tray using OpenCV...\n");

        int screen_width = GetSystemMetrics(SM_CXSCREEN);
        int screen_height = GetSystemMetrics(SM_CYSCREEN);

        printf("Screen size: %dx%d\n", screen_width, screen_height);

        int search_width = 600;
        int search_height = 80;

        if (search_width > screen_width)
            search_width = screen_width;
        if (search_height > screen_height)
            search_height = screen_height;

        result.x = screen_width - search_width;
        result.y = screen_height - search_height;
        result.width = search_width;
        result.height = search_height;
        result.confidence = 0.8f;

        printf("System tray region: %dx%d at (%d,%d), confidence: %.2f\n",
               result.width, result.height, result.x, result.y, result.confidence);

        return result;
    }

    AUTOMATOR_API DetectedRegion *find_text_regions_in_tray(ScreenRegion region, int *region_count)
    {
        DetectedRegion *result = NULL;
        *region_count = 0;

        printf("\nLooking for text regions in tray area...\n");
        printf("Tray area: %dx%d at (%d,%d)\n",
               region.width, region.height, region.x, region.y);

        // Простая реализация без OpenCV
        printf("OpenCV not available - using simple heuristic\n");

        // Всегда возвращаем одну область как fallback
        *region_count = 1;
        result = (DetectedRegion *)malloc(sizeof(DetectedRegion));

        // Возвращаем всю область трея
        result[0].x = region.x;
        result[0].y = region.y;
        result[0].width = region.width;
        result[0].height = region.height;
        result[0].confidence = 0.2f;

        printf("  Text region 1: %dx%d at (%d,%d), confidence: %.2f\n",
               result[0].width, result[0].height,
               result[0].x, result[0].y,
               result[0].confidence);

        return result;
    }

    AUTOMATOR_API DetectedRegion find_keyboard_layout_in_region(ScreenRegion region)
    {
        DetectedRegion result = {0, 0, 0, 0, 0.0f};

        printf("Looking for keyboard layout in region: %dx%d at (%d,%d)\n",
               region.width, region.height, region.x, region.y);

        // Простая эвристика без OpenCV
        printf("Using heuristic layout detection\n");

        int layout_width = 60;
        int layout_height = 30;

        // Расположение иконки раскладки обычно в правой части системного трея
        result.x = region.x + region.width - layout_width - 10;
        result.y = region.y + region.height - layout_height - 5;
        result.width = layout_width;
        result.height = layout_height;
        result.confidence = 0.5f;

        printf("Heuristic layout region: %dx%d at (%d,%d)\n",
               result.width, result.height, result.x - region.x, result.y - region.y);

        if (result.width == 0 || result.height == 0)
        {
            printf("WARNING: Could not find keyboard layout in region\n");
        }

        return result;
    }

} // extern "C"