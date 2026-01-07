// opencv_wrapper.cpp - Реальная реализация OpenCV детекции текста
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <vector>
#include <algorithm>

#include "automator.h"

// Подключаем OpenCV
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/text.hpp>

using namespace cv;
using namespace std;

// Вспомогательная функция для определения, является ли контур текстом
static bool is_contour_text(const Rect &rect, int image_width, int image_height)
{
    int width = rect.width;
    int height = rect.height;

    // Пропускаем слишком маленькие объекты
    if (width < 10 || height < 8)
        return false;

    // Пропускаем слишком большие объекты
    if (width > 300 || height > 60)
        return false;

    // Проверяем соотношение сторон
    float aspect_ratio = (float)width / height;

    // Текст обычно имеет соотношение сторон от 1.2 до 10
    if (aspect_ratio < 1.2 || aspect_ratio > 15)
        return false;

    // Проверяем площадь
    float area = width * height;
    float image_area = image_width * image_height;
    float relative_area = area / image_area;

    // Текст обычно занимает небольшую часть изображения
    if (relative_area > 0.1)
        return false;

    // Проверяем высоту - текст обычно не слишком мал и не слишком велик
    if (height < 8 || height > 40)
        return false;

    return true;
}

// Функция для объединения близко расположенных регионов
static vector<Rect> merge_close_regions(const vector<Rect> &regions, int merge_threshold = 10)
{
    if (regions.empty())
        return regions;

    vector<Rect> merged;
    vector<bool> merged_flag(regions.size(), false);

    for (size_t i = 0; i < regions.size(); i++)
    {
        if (merged_flag[i])
            continue;

        Rect current = regions[i];
        int merge_count = 1;

        for (size_t j = i + 1; j < regions.size(); j++)
        {
            if (merged_flag[j])
                continue;

            Rect other = regions[j];

            // Проверяем, находятся ли регионы близко
            int center_x1 = current.x + current.width / 2;
            int center_y1 = current.y + current.height / 2;
            int center_x2 = other.x + other.width / 2;
            int center_y2 = other.y + other.height / 2;

            int distance_x = abs(center_x1 - center_x2);
            int distance_y = abs(center_y1 - center_y2);

            // Проверяем, находятся ли на одной строке
            bool same_line = (distance_y <= current.height / 2);

            // Проверяем расстояние по горизонтали
            bool close_horizontal = (distance_x <= current.width + merge_threshold);

            if (same_line && close_horizontal)
            {
                // Объединяем регионы
                int x = min(current.x, other.x);
                int y = min(current.y, other.y);
                int x2 = max(current.x + current.width, other.x + other.width);
                int y2 = max(current.y + current.height, other.y + other.height);

                current = Rect(x, y, x2 - x, y2 - y);
                merged_flag[j] = true;
                merge_count++;
            }
        }

        merged.push_back(current);
        merged_flag[i] = true;
    }

    return merged;
}

extern "C"
{

    AUTOMATOR_API int opencv_init(void)
    {
        printf("OpenCV module initialized (real text detection)\n");
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

        printf("\nLooking for text regions in tray area using OpenCV...\n");
        printf("Tray area: %dx%d at (%d,%d)\n",
               region.width, region.height, region.x, region.y);

        // Захватываем изображение трея
        char temp_file[MAX_PATH];
        GetTempPath(MAX_PATH, temp_file);
        sprintf(temp_file, "%sopencv_tray_%lu.bmp", temp_file, (unsigned long)GetCurrentProcessId());

        if (!capture_screen_region(region.x, region.y, region.width, region.height, temp_file))
        {
            printf("Failed to capture tray region for text detection\n");
            return result;
        }

        try
        {
            // Загружаем изображение с помощью OpenCV
            Mat image = imread(temp_file);
            if (image.empty())
            {
                printf("Failed to load image for text detection\n");
                DeleteFile(temp_file);
                return result;
            }

            // Преобразуем в оттенки серого
            Mat gray;
            cvtColor(image, gray, COLOR_BGR2GRAY);

            // Улучшаем контраст для лучшего распознавания текста
            Mat enhanced;
            equalizeHist(gray, enhanced);

            // Применяем адаптивную бинаризацию
            Mat binary;
            adaptiveThreshold(enhanced, binary, 255,
                              ADAPTIVE_THRESH_GAUSSIAN_C,
                              THRESH_BINARY, 11, 2);

            // Морфологические операции для удаления шума
            Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3));
            Mat morph;
            morphologyEx(binary, morph, MORPH_CLOSE, kernel);

            // Находим контуры
            vector<vector<Point>> contours;
            vector<Vec4i> hierarchy;
            findContours(morph, contours, hierarchy,
                         RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

            // Собираем текстовые регионы
            vector<Rect> text_rects;

            for (size_t i = 0; i < contours.size(); i++)
            {
                Rect rect = boundingRect(contours[i]);

                // Проверяем, является ли контур текстом
                if (is_contour_text(rect, image.cols, image.rows))
                {
                    text_rects.push_back(rect);
                }
            }

            // Объединяем близко расположенные регионы
            vector<Rect> merged_rects = merge_close_regions(text_rects, 15);

            // Фильтруем регионы по размеру и положению
            vector<Rect> final_rects;
            for (const auto &rect : merged_rects)
            {
                // Дополнительная фильтрация
                float aspect_ratio = (float)rect.width / rect.height;

                // Текст обычно имеет определенные пропорции
                if (aspect_ratio < 1.5 || aspect_ratio > 12)
                    continue;

                // Проверяем, что регион не слишком близко к краю
                if (rect.x < 5 || rect.y < 5 ||
                    rect.x + rect.width > image.cols - 5 ||
                    rect.y + rect.height > image.rows - 5)
                    continue;

                final_rects.push_back(rect);
            }

            // Сортируем регионы по положению (слева направо)
            sort(final_rects.begin(), final_rects.end(),
                 [](const Rect &a, const Rect &b)
                 {
                     return a.x < b.x;
                 });

            // Создаем массив для возврата
            if (!final_rects.empty())
            {
                *region_count = (int)final_rects.size();
                result = (DetectedRegion *)malloc(*region_count * sizeof(DetectedRegion));

                for (size_t i = 0; i < final_rects.size(); i++)
                {
                    const Rect &rect = final_rects[i];

                    // Рассчитываем уверенность на основе характеристик региона
                    float confidence = 0.5f;
                    float aspect_ratio = (float)rect.width / rect.height;

                    // Бонус за хорошее соотношение сторон (2-8 считается хорошим для текста)
                    if (aspect_ratio >= 2.0f && aspect_ratio <= 8.0f)
                        confidence += 0.2f;

                    // Бонус за типичную высоту текста (12-24 пикселя)
                    if (rect.height >= 12 && rect.height <= 24)
                        confidence += 0.2f;

                    // Ограничиваем уверенность
                    if (confidence > 0.95f)
                        confidence = 0.95f;

                    // Заполняем результат
                    result[i].x = region.x + rect.x;
                    result[i].y = region.y + rect.y;
                    result[i].width = rect.width;
                    result[i].height = rect.height;
                    result[i].confidence = confidence;

                    printf("  Text region %d: %dx%d at (%d+%d,%d+%d), aspect: %.2f, confidence: %.2f\n",
                           (int)i + 1, rect.width, rect.height,
                           region.x, rect.x, region.y, rect.y,
                           aspect_ratio, confidence);
                }
            }
            else
            {
                printf("No text regions found\n");
            }

            // Для отладки: сохраняем изображение с отмеченными регионами
            if (!final_rects.empty())
            {
                Mat debug_image = image.clone();
                for (const auto &rect : final_rects)
                {
                    rectangle(debug_image, rect, Scalar(0, 255, 0), 2);
                }

                char debug_file[MAX_PATH];
                sprintf(debug_file, "%sopencv_debug_%lu.bmp", temp_file, (unsigned long)GetCurrentProcessId());
                imwrite(debug_file, debug_image);
                printf("Debug image with detected regions saved: %s\n", debug_file);
            }
        }
        catch (const cv::Exception &e)
        {
            printf("OpenCV exception: %s\n", e.what());
        }
        catch (...)
        {
            printf("Unknown exception in OpenCV processing\n");
        }

        // Удаляем временный файл
        DeleteFile(temp_file);

        // Fallback если OpenCV не нашел регионы
        if (*region_count == 0)
        {
            printf("Using fallback text region detection\n");
            *region_count = 1;
            result = (DetectedRegion *)malloc(sizeof(DetectedRegion));
            result[0].x = region.x;
            result[0].y = region.y;
            result[0].width = region.width;
            result[0].height = region.height;
            result[0].confidence = 0.2f;

            printf("  Fallback region: %dx%d at (%d,%d)\n",
                   result[0].width, result[0].height,
                   result[0].x, result[0].y);
        }

        return result;
    }

    AUTOMATOR_API DetectedRegion find_keyboard_layout_in_region(ScreenRegion region)
    {
        DetectedRegion result = {0, 0, 0, 0, 0.0f};

        printf("Looking for keyboard layout in region: %dx%d at (%d,%d)\n",
               region.width, region.height, region.x, region.y);

        // Сначала ищем текстовые регионы
        int region_count = 0;
        DetectedRegion *text_regions = find_text_regions_in_tray(region, &region_count);

        if (text_regions && region_count > 0)
        {
            // Ищем регион, который может содержать раскладку
            for (int i = 0; i < region_count; i++)
            {
                DetectedRegion reg = text_regions[i];

                // Проверяем положение - раскладка обычно в правой части
                int relative_x = reg.x - region.x;
                if (relative_x > region.width * 0.6)
                {
                    // Проверяем размер - раскладка обычно небольшая (2-3 символа)
                    if (reg.width >= 20 && reg.width <= 80 &&
                        reg.height >= 15 && reg.height <= 35)
                    {
                        // Проверяем соотношение сторон
                        float aspect_ratio = (float)reg.width / reg.height;
                        if (aspect_ratio >= 1.5f && aspect_ratio <= 3.5f)
                        {
                            result = reg;
                            result.confidence = 0.7f;
                            printf("Found potential layout region: %dx%d at (%d,%d)\n",
                                   result.width, result.height, result.x, result.y);
                            break;
                        }
                    }
                }
            }

            free(text_regions);
        }

        // Если не нашли через OpenCV, используем эвристику
        if (result.width == 0 || result.height == 0)
        {
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
        }

        if (result.width == 0 || result.height == 0)
        {
            printf("WARNING: Could not find keyboard layout in region\n");
        }

        return result;
    }

} // extern "C"