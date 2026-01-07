#ifndef AUTOMATOR_H
#define AUTOMATOR_H

/* Общий макрос для удобства */
#ifdef BUILDING_AUTOMATOR_DLL
#define AUTOMATOR_API __declspec(dllexport)
#else
#define AUTOMATOR_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#include "screen.h"
#include "opencv.h"
#include "ocr.h"

    /* Функции для работы с экраном */
    AUTOMATOR_API int capture_screen_region(int x, int y, int width, int height, const char *filename);
    AUTOMATOR_API HBITMAP capture_to_bitmap(int x, int y, int width, int height);
    AUTOMATOR_API int save_bitmap_to_file(HBITMAP hBitmap, const char *filename);
    AUTOMATOR_API int get_screen_width(void);
    AUTOMATOR_API int get_screen_height(void);
    AUTOMATOR_API ScreenRegion get_system_tray_region(void);

    /* Функции OpenCV */
    AUTOMATOR_API int opencv_init(void);
    AUTOMATOR_API void opencv_cleanup(void);
    AUTOMATOR_API DetectedRegion detect_system_tray_region_opencv(void);
    AUTOMATOR_API DetectedRegion *find_text_regions_in_tray(ScreenRegion region, int *region_count);
    AUTOMATOR_API DetectedRegion find_keyboard_layout_in_region(ScreenRegion region);

    /* Функции OCR */
    AUTOMATOR_API int ocr_init(const char *tessdata_path);
    AUTOMATOR_API void ocr_cleanup(void);
    AUTOMATOR_API OCRResult ocr_from_file(const char *image_path, const char *language, const unsigned int psm, const unsigned int dpi);
    AUTOMATOR_API void ocr_result_free(OCRResult *result);

#ifdef __cplusplus
}
#endif

#endif
