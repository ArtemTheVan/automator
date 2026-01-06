#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "screen.h"
#include "opencv.h"

/* Включаем заголовки OpenCV C API */
#ifdef HAVE_OPENCV
#include <opencv2/core/core_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/highgui/highgui_c.h>
#endif

/* Простая функция для поиска окна VM */
static HWND find_vm_window_simple(void)
{
    const char *vm_titles[] = {"VirtualBox", "VMware", "Virtual Machine", NULL};
    for (int i = 0; vm_titles[i]; i++)
    {
        HWND hwnd = FindWindow(NULL, vm_titles[i]);
        if (hwnd && IsWindowVisible(hwnd))
        {
            return hwnd;
        }
    }
    return NULL;
}

/* Реализация функций OpenCV */
int opencv_init(void)
{
    printf("OpenCV module initialized (C API)\n");
    return 1;
}

void opencv_cleanup(void)
{
    printf("OpenCV module cleaned up\n");
}

DetectedRegion detect_system_tray_region_opencv(void)
{
    DetectedRegion result = {0};

    printf("Detecting system tray using OpenCV C API...\n");

    /* Пытаемся найти окно VM */
    HWND vm_window = find_vm_window_simple();
    ScreenRegion search_region;

    if (vm_window)
    {
        RECT rect;
        GetWindowRect(vm_window, &rect);
        search_region.x = rect.left;
        search_region.y = rect.top;
        search_region.width = rect.right - rect.left;
        search_region.height = rect.bottom - rect.top;
        printf("Found VM window: %dx%d at (%d,%d)\n",
               search_region.width, search_region.height,
               search_region.x, search_region.y);
    }
    else
    {
        /* Используем весь экран */
        search_region.x = 0;
        search_region.y = 0;
        search_region.width = GetSystemMetrics(SM_CXSCREEN);
        search_region.height = GetSystemMetrics(SM_CYSCREEN);
        printf("Using full screen: %dx%d\n",
               search_region.width, search_region.height);
    }

    /* Захватываем правую нижнюю часть для поиска трея */
    int tray_width = 300;
    int tray_height = 100;
    int tray_x = search_region.x + search_region.width - tray_width;
    int tray_y = search_region.y + search_region.height - tray_height;

    /* Убедимся, что не вышли за границы */
    if (tray_x < search_region.x)
        tray_x = search_region.x;
    if (tray_y < search_region.y)
        tray_y = search_region.y;

    result.x = tray_x;
    result.y = tray_y;
    result.width = tray_width;
    result.height = tray_height;
    result.confidence = 0.7;

    printf("OpenCV detected system tray region: %dx%d at (%d,%d), confidence: %.2f\n",
           result.width, result.height, result.x, result.y, result.confidence);

    return result;
}

/* Находит область с текстом раскладки клавиатуры в заданном регионе */
DetectedRegion find_keyboard_layout_in_region(ScreenRegion region)
{
    DetectedRegion result = {0};

    printf("Looking for keyboard layout in region: %dx%d at (%d,%d)\n",
           region.width, region.height, region.x, region.y);

#ifdef HAVE_OPENCV
    /* Захватываем регион экрана */
    char temp_file[MAX_PATH];
    GetTempPath(MAX_PATH, temp_file);
    sprintf(temp_file, "%sopencv_temp_%lu.bmp", temp_file, GetCurrentProcessId());

    if (!capture_screen_region(region.x, region.y, region.width, region.height, temp_file))
    {
        printf("Failed to capture region for OpenCV\n");
        return result;
    }

    /* Загружаем изображение с помощью OpenCV C API */
    IplImage *src = cvLoadImage(temp_file, CV_LOAD_IMAGE_COLOR);
    if (!src)
    {
        printf("Failed to load image for layout detection\n");
        DeleteFile(temp_file);
        return result;
    }

    /* Преобразуем в grayscale */
    IplImage *gray = cvCreateImage(cvGetSize(src), IPL_DEPTH_8U, 1);
    cvCvtColor(src, gray, CV_BGR2GRAY);

    /* Применяем бинаризацию для улучшения контраста */
    IplImage *binary = cvCreateImage(cvGetSize(gray), IPL_DEPTH_8U, 1);
    cvThreshold(gray, binary, 200, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);

    /* Находим контуры */
    CvMemStorage *storage = cvCreateMemStorage(0);
    CvSeq *contours = NULL;
    cvFindContours(binary, storage, &contours, sizeof(CvContour),
                   CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cvPoint(0, 0));

    /* Ищем контуры, которые могут быть текстом раскладки */
    double max_area = 0;
    CvRect best_rect = cvRect(0, 0, 0, 0);

    for (CvSeq *c = contours; c != NULL; c = c->h_next)
    {
        CvRect rect = cvBoundingRect(c, 0);

        /* Фильтруем по размеру - текст раскладки обычно небольшой */
        if (rect.width > 20 && rect.width < 100 &&
            rect.height > 15 && rect.height < 50)
        {
            /* Проверяем соотношение сторон - текст обычно шире, чем высота */
            float aspect_ratio = (float)rect.width / rect.height;
            if (aspect_ratio > 1.5 && aspect_ratio < 5.0)
            {
                /* Проверяем положение - обычно текст в правой части трея */
                int rel_x = rect.x;
                int rel_y = rect.y;

                /* Если контур находится в нижней половине изображения */
                if (rel_y > region.height / 2 ||
                    (rel_x > region.width * 0.6 && rel_y > region.height * 0.3))
                {
                    double area = rect.width * rect.height;
                    if (area > max_area)
                    {
                        max_area = area;
                        best_rect = rect;
                    }
                }
            }
        }
    }

    /* Если нашли подходящий контур */
    if (best_rect.width > 0 && best_rect.height > 0)
    {
        /* Преобразуем координаты обратно в экранные */
        result.x = region.x + best_rect.x;
        result.y = region.y + best_rect.y;
        result.width = best_rect.width;
        result.height = best_rect.height;
        result.confidence = 0.8f;

        printf("Found keyboard layout contour: %dx%d at (%d,%d) within region\n",
               result.width, result.height, best_rect.x, best_rect.y);
    }
    else
    {
        printf("No contours found matching criteria\n");
    }

    /* Освобождаем ресурсы OpenCV */
    cvReleaseMemStorage(&storage);
    cvReleaseImage(&binary);
    cvReleaseImage(&gray);
    cvReleaseImage(&src);

    /* Удаляем временный файл */
    DeleteFile(temp_file);
#else
    /* Простой алгоритм, если OpenCV не доступен */
    int layout_width = 60;
    int layout_height = 30;

    /* Расположение иконки раскладки обычно в правой части системного трея */
    result.x = region.x + region.width - layout_width - 10;
    result.y = region.y + region.height - layout_height - 5;
    result.width = layout_width;
    result.height = layout_height;
    result.confidence = 0.5f;

    printf("Using simple layout detection: %dx%d at (%d,%d)\n",
           result.width, result.height, result.x - region.x, result.y - region.y);
#endif

    if (result.width == 0 || result.height == 0)
    {
        printf("WARNING: Could not find keyboard layout in region\n");
    }

    return result;
}