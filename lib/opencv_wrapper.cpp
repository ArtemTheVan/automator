#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>

#include "automator.h"
#include "log.h"

#ifdef HAVE_OPENCV

#include <vector>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <set>

// Подключаем OpenCV
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

using namespace cv;
using namespace std;

// Конфигурация для максимальной детекции
struct Config
{
    // Основные параметры
    int min_width = 5;
    int min_height = 5;
    int max_width = 500;
    int max_height = 100;

    // Параметры для объединения
    int max_horizontal_gap = 10;
    int max_vertical_gap = 5;

    // Параметры морфологических операций
    int morph_horizontal_size = 8;
    int morph_vertical_size = 1;

    // Минимальная площадь контура
    int min_contour_area = 20;
};

static Config config;

// Функция для быстрого объединения регионов
static vector<Rect> simple_merge_regions(const vector<Rect> &regions)
{
    if (regions.size() <= 1)
        return regions;

    vector<Rect> result;
    vector<bool> merged(regions.size(), false);

    for (size_t i = 0; i < regions.size(); i++)
    {
        if (merged[i])
            continue;

        Rect current = regions[i];

        for (size_t j = i + 1; j < regions.size(); j++)
        {
            if (merged[j])
                continue;

            Rect other = regions[j];

            // Проверяем расстояние между регионами
            int left = max(current.x, other.x);
            int right = min(current.x + current.width, other.x + other.width);
            int top = max(current.y, other.y);
            int bottom = min(current.y + current.height, other.y + other.height);

            // Если регионы пересекаются или очень близки
            if (right >= left && bottom >= top)
            {
                // Пересекаются - объединяем
                current = current | other;
                merged[j] = true;
            }
            else
            {
                // Проверяем расстояние
                int horizontal_gap = max(0, max(current.x, other.x) - min(current.x + current.width, other.x + other.width));
                int vertical_gap = max(0, max(current.y, other.y) - min(current.y + current.height, other.y + other.height));

                if (horizontal_gap <= config.max_horizontal_gap && vertical_gap <= config.max_vertical_gap)
                {
                    current = current | other;
                    merged[j] = true;
                }
            }
        }

        result.push_back(current);
        merged[i] = true;
    }

    printf("Simple merge: %d -> %d regions\n", (int)regions.size(), (int)result.size());
    return result;
}

// Основная функция детекции текста - максимально простая
static vector<pair<Rect, float>> detect_text_regions(const Mat &image)
{
    vector<pair<Rect, float>> result;

    printf("=== Simple Text Detection (Maximum) ===\n");
    printf("Image size: %dx%d\n", image.cols, image.rows);

    // Простое преобразование в серый
    Mat gray;
    if (image.channels() == 3)
    {
        cvtColor(image, gray, COLOR_BGR2GRAY);
    }
    else
    {
        gray = image.clone();
    }

    // 1. Простая бинаризация
    Mat binary1;
    threshold(gray, binary1, 128, 255, THRESH_BINARY);

    // 2. Адаптивная бинаризация
    Mat binary2;
    adaptiveThreshold(gray, binary2, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 11, 2);

    // 3. Инвертированная адаптивная бинаризация
    Mat binary3;
    adaptiveThreshold(gray, binary3, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY_INV, 11, 2);

    // Объединяем все бинарные изображения
    Mat combined = Mat::zeros(gray.size(), CV_8U);
    bitwise_or(combined, binary1, combined);
    bitwise_or(combined, binary2, combined);
    bitwise_or(combined, binary3, combined);

    // Морфологические операции для соединения
    Mat morph;
    Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3));
    morphologyEx(combined, morph, MORPH_CLOSE, kernel);

    // Находим контуры
    vector<vector<Point>> contours;
    findContours(morph, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    printf("Found %d contours\n", (int)contours.size());

    vector<Rect> all_regions;

    // Минимальная фильтрация
    for (const auto &contour : contours)
    {
        Rect rect = boundingRect(contour);

        // Очень базовые проверки
        if (rect.width < config.min_width || rect.height < config.min_height)
            continue;

        if (rect.width > config.max_width || rect.height > config.max_height)
            continue;

        double area = contourArea(contour);
        if (area < config.min_contour_area)
            continue;

        // Добавляем небольшой отступ
        int padding = 1;
        Rect padded_rect(
            max(0, rect.x - padding),
            max(0, rect.y - padding),
            min(image.cols - rect.x, rect.width + 2 * padding),
            min(image.rows - rect.y, rect.height + 2 * padding));

        all_regions.push_back(padded_rect);
    }

    printf("Found %d regions after basic filtering\n", (int)all_regions.size());

    // Если не нашли достаточно регионов, пробуем другой метод
    if (all_regions.size() < 5)
    {
        printf("Too few regions, trying alternative method...\n");

        // Альтернативный метод: детекция краев
        Mat edges;
        Canny(gray, edges, 30, 100);

        // Дилатация для соединения
        Mat dilated;
        dilate(edges, dilated, getStructuringElement(MORPH_RECT, Size(2, 2)));

        vector<vector<Point>> edge_contours;
        findContours(dilated, edge_contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        for (const auto &contour : edge_contours)
        {
            Rect rect = boundingRect(contour);

            if (rect.width < 5 || rect.height < 5)
                continue;

            if (rect.width > 400 || rect.height > 80)
                continue;

            all_regions.push_back(rect);
        }

        printf("Now have %d regions total\n", (int)all_regions.size());
    }

    if (all_regions.empty())
    {
        printf("No regions found\n");
        return result;
    }

    // Объединяем регионы
    vector<Rect> merged_regions = simple_merge_regions(all_regions);

    // Оцениваем регионы
    for (const Rect &rect : merged_regions)
    {
        // Простая оценка на основе размера и формы
        float confidence = 0.5f;

        float aspect_ratio = (float)rect.width / rect.height;

        // Регионы с хорошим соотношением сторон для текста
        if (aspect_ratio > 1.2f && aspect_ratio < 8.0f)
            confidence += 0.3f;

        // Более крупные регионы получают более высокую оценку
        if (rect.width > 30 && rect.height > 15)
            confidence += 0.2f;

        confidence = min(confidence, 0.95f);

        result.push_back({rect, confidence});
    }

    // Сортируем по положению
    sort(result.begin(), result.end(),
         [](const pair<Rect, float> &a, const pair<Rect, float> &b)
         {
             if (abs(a.first.y - b.first.y) > 20)
                 return a.first.y < b.first.y;
             return a.first.x < b.first.x;
         });

    printf("Final result: %d text regions\n", (int)result.size());

    // Сохраняем отладочное изображение
    if (!result.empty())
    {
        Mat debug = image.clone();
        for (size_t i = 0; i < result.size(); i++)
        {
            rectangle(debug, result[i].first, Scalar(0, 255, 0), 2);
        }
        imwrite("debug_all_regions.bmp", debug);
        printf("Debug image saved: debug_all_regions.bmp\n");
    }

    return result;
}

extern "C"
{

    AUTOMATOR_API int opencv_init(void)
    {
        LOG_INFO("OpenCV Text Detection initialized (config: min=%dx%d, max=%dx%d)",
                 config.min_width, config.min_height,
                 config.max_width, config.max_height);
        return 1;
    }

    AUTOMATOR_API void opencv_cleanup(void)
    {
        LOG_DEBUG("OpenCV module cleaned up");
    }

    AUTOMATOR_API DetectedRegion detect_system_tray_region_opencv(void)
    {
        DetectedRegion result = {0, 0, 0, 0, 0.0f};

        printf("Detecting system tray region...\n");

        int screen_width = GetSystemMetrics(SM_CXSCREEN);
        int screen_height = GetSystemMetrics(SM_CYSCREEN);

        printf("Screen size: %dx%d\n", screen_width, screen_height);

        int tray_width = min(800, screen_width);
        int tray_height = min(120, screen_height);

        result.x = screen_width - tray_width;
        result.y = screen_height - tray_height;
        result.width = tray_width;
        result.height = tray_height;
        result.confidence = 0.9f;

        printf("System tray region: %dx%d at (%d,%d)\n",
               result.width, result.height, result.x, result.y);

        return result;
    }

    AUTOMATOR_API DetectedRegion *find_text_regions_in_tray(ScreenRegion region, int *region_count)
    {
        DetectedRegion *result = NULL;
        *region_count = 0;

        printf("\n=== Text Detection in System Tray ===\n");
        printf("Search region: %dx%d at (%d,%d)\n",
               region.width, region.height, region.x, region.y);

        // Захватываем изображение
        char temp_dir[MAX_PATH];
        char temp_file[MAX_PATH];
        GetTempPath(MAX_PATH, temp_dir);
        snprintf(temp_file, sizeof(temp_file), "%sopencv_%lu.bmp",
                 temp_dir, (unsigned long)GetCurrentProcessId());

        if (!capture_screen_region(region.x, region.y, region.width, region.height, temp_file))
        {
            printf("ERROR: Failed to capture screen region\n");
            return result;
        }

        try
        {
            // Загружаем изображение
            Mat image = imread(temp_file, IMREAD_COLOR);
            if (image.empty())
            {
                printf("ERROR: Failed to load captured image\n");
                DeleteFile(temp_file);
                return result;
            }

            printf("Input image: %dx%d\n", image.cols, image.rows);

            // Детекция текста
            vector<pair<Rect, float>> text_regions = detect_text_regions(image);

            // Создаем результат
            if (!text_regions.empty())
            {
                *region_count = (int)text_regions.size();
                result = (DetectedRegion *)malloc(*region_count * sizeof(DetectedRegion));

                // Создаем debug изображения
                Mat debug_image = image.clone();
                Mat numbered_image = image.clone();

                for (size_t i = 0; i < text_regions.size(); i++)
                {
                    const Rect &rect = text_regions[i].first;
                    float confidence = text_regions[i].second;

                    // Добавляем отступ для OCR
                    int padding = 1;
                    Rect padded_rect(
                        max(0, rect.x - padding),
                        max(0, rect.y - padding),
                        min(image.cols - rect.x, rect.width + 2 * padding),
                        min(image.rows - rect.y, rect.height + 2 * padding));

                    result[i].x = region.x + padded_rect.x;
                    result[i].y = region.y + padded_rect.y;
                    result[i].width = padded_rect.width;
                    result[i].height = padded_rect.height;
                    result[i].confidence = confidence;

                    // Выводим информацию
                    float aspect_ratio = (float)padded_rect.width / padded_rect.height;
                    printf("  Region %3d: %4dx%3d at (+%3d,+%3d), aspect: %5.2f, conf: %.2f\n",
                           (int)i + 1, padded_rect.width, padded_rect.height,
                           padded_rect.x, padded_rect.y, aspect_ratio, confidence);

                    // Визуализация
                    rectangle(debug_image, padded_rect, Scalar(0, 255, 0), 1);
                    rectangle(numbered_image, padded_rect, Scalar(0, 255, 0), 1);

                    char label[20];
                    snprintf(label, sizeof(label), "#%d", (int)i + 1);
                    putText(numbered_image, label,
                            Point(padded_rect.x + 2, padded_rect.y + 12),
                            FONT_HERSHEY_SIMPLEX, 0.4, Scalar(0, 0, 255), 1);
                }

                printf("TOTAL DETECTED REGIONS: %d\n", *region_count);

                // Сохраняем debug изображения (используем temp_dir, а не temp_file).
                char debug_file[MAX_PATH];
                snprintf(debug_file, sizeof(debug_file), "%sdebug_text_region_%lu.bmp",
                         temp_dir, (unsigned long)GetCurrentProcessId());
                imwrite(debug_file, debug_image);

                char numbered_file[MAX_PATH];
                snprintf(numbered_file, sizeof(numbered_file), "%sdebug_text_numbered_%lu.bmp",
                         temp_dir, (unsigned long)GetCurrentProcessId());
                imwrite(numbered_file, numbered_image);

                printf("Debug images saved:\n");
                printf("  - %s (with borders)\n", debug_file);
                printf("  - %s (with numbers)\n", numbered_file);
            }
            else
            {
                printf("No text regions detected\n");
            }
        }
        catch (const cv::Exception &e)
        {
            printf("OpenCV Exception: %s\n", e.what());
        }
        catch (...)
        {
            printf("Unknown exception in OpenCV processing\n");
        }

        DeleteFile(temp_file);

        return result;
    }

    AUTOMATOR_API DetectedRegion find_keyboard_layout_in_region(ScreenRegion region)
    {
        DetectedRegion result = {0, 0, 0, 0, 0.0f};

        printf("\nLooking for keyboard layout...\n");
        printf("Search region: %dx%d at (%d,%d)\n",
               region.width, region.height, region.x, region.y);

        // Ищем текстовые регионы
        int region_count = 0;
        DetectedRegion *text_regions = find_text_regions_in_tray(region, &region_count);

        if (text_regions && region_count > 0)
        {
            // Ищем регион с характеристиками раскладки
            for (int i = 0; i < region_count; i++)
            {
                DetectedRegion reg = text_regions[i];

                // Правый нижний угол
                int relative_x = reg.x - region.x;
                int relative_y = reg.y - region.y;

                if (relative_x > region.width * 0.7 && relative_y > region.height * 0.6)
                {
                    // Размер для 2-3 символов
                    if (reg.width >= 15 && reg.width <= 80 &&
                        reg.height >= 10 && reg.height <= 30)
                    {
                        float aspect_ratio = (float)reg.width / reg.height;
                        if (aspect_ratio >= 1.0f && aspect_ratio <= 5.0f)
                        {
                            result = reg;
                            result.confidence = max(reg.confidence, 0.7f);
                            printf("Found keyboard layout: %dx%d at (%d,%d)\n",
                                   result.width, result.height, result.x, result.y);
                            break;
                        }
                    }
                }
            }

            free(text_regions);
        }

        // Эвристика
        if (result.width == 0)
        {
            int layout_width = 45;
            int layout_height = 20;

            result.x = region.x + region.width - layout_width - 8;
            result.y = region.y + region.height - layout_height - 6;
            result.width = layout_width;
            result.height = layout_height;
            result.confidence = 0.4f;
        }

        return result;
    }

} // extern "C"

#else /* !HAVE_OPENCV */

/*
 * Заглушки на случай, когда OpenCV недоступен в сборке.
 * Они нужны, чтобы DLL экспортировала тот же набор функций
 * и вызывающий код компилировался без условной компиляции.
 */
extern "C" {

AUTOMATOR_API int opencv_init(void)
{
    LOG_WARN("OpenCV support not compiled in (build with HAVE_OPENCV=1)");
    return 0;
}

AUTOMATOR_API void opencv_cleanup(void) {}

AUTOMATOR_API DetectedRegion detect_system_tray_region_opencv(void)
{
    DetectedRegion r = {0, 0, 0, 0, 0.0f};
    return r;
}

AUTOMATOR_API DetectedRegion *find_text_regions_in_tray(ScreenRegion region, int *region_count)
{
    (void)region;
    if (region_count) *region_count = 0;
    return NULL;
}

AUTOMATOR_API DetectedRegion find_keyboard_layout_in_region(ScreenRegion region)
{
    (void)region;
    DetectedRegion r = {0, 0, 0, 0, 0.0f};
    return r;
}

} // extern "C"

#endif /* HAVE_OPENCV */
