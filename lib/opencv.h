#ifndef OPENCV_H_
#define OPENCV_H_

#include "screen.h"

/* Макросы для экспорта функций из DLL */
#ifdef BUILDING_AUTOMATOR_DLL
#define OPENCV_API __declspec(dllexport)
#else
#define OPENCV_API __declspec(dllimport)
#endif

/* Структура для обнаруженной области */
typedef struct
{
    int x;
    int y;
    int width;
    int height;
    float confidence;
} DetectedRegion;

#ifdef __cplusplus
extern "C"
{
#endif

    /* Функции для работы с OpenCV */
    OPENCV_API int opencv_init(void);
    OPENCV_API void opencv_cleanup(void);
    OPENCV_API DetectedRegion detect_system_tray_region_opencv(void);
    OPENCV_API DetectedRegion *find_text_regions_in_tray(ScreenRegion region, int *region_count);
    OPENCV_API DetectedRegion find_keyboard_layout_in_region(ScreenRegion region);

#ifdef __cplusplus
}
#endif

#endif
