#ifndef AUTOMATOR_H
#define AUTOMATOR_H

/* Общий макрос экспорта */
#ifdef BUILDING_AUTOMATOR_DLL
#define AUTOMATOR_API __declspec(dllexport)
#else
#define AUTOMATOR_API __declspec(dllimport)
#endif

/* Версия библиотеки. AUTOMATOR_VERSION может быть переопределён сборкой
   (например, -DAUTOMATOR_VERSION=\"1.2.3\"); по умолчанию — здесь. */
#ifndef AUTOMATOR_VERSION
#define AUTOMATOR_VERSION "0.2.0"
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#include "keyboard.h"
#include "mouse.h"
#include "screen.h"
#include "opencv.h"
#include "ocr.h"

    /* Версия библиотеки (строка в формате SemVer). */
    AUTOMATOR_API const char *automator_version(void);

    /* Включить per-monitor DPI awareness и многомониторную систему координат
       (виртуальный экран). Безопасно вызывать многократно; вызвать один раз
       в начале программы. Возвращает 1 при успехе, 0 если функция не
       поддерживается данной версией Windows (тогда продолжаем без DPI awareness). */
    AUTOMATOR_API int automator_init(void);

    /* Настройка задержек (мс). 0 — мгновенно. Значения по умолчанию:
       keystroke = 10 мс, mouse = 50 мс. */
    AUTOMATOR_API void set_keystroke_delay_ms(int ms);
    AUTOMATOR_API void set_mouse_delay_ms(int ms);

    /* Функции для работы с клавиатурой */
    AUTOMATOR_API void simulate_keystroke(const char *text);

    /* Функции для работы с мышью */
    AUTOMATOR_API void simulate_mouse_sequence(const MouseAction *actions, int count);
    AUTOMATOR_API void simulate_mouse_click_at(int x, int y);

    /* Функции для работы с экраном */
    AUTOMATOR_API int capture_screen_region(int x, int y, int width, int height, const char *filename);
    AUTOMATOR_API HBITMAP capture_to_bitmap(int x, int y, int width, int height);
    AUTOMATOR_API int save_bitmap_to_file(HBITMAP hBitmap, const char *filename);
    AUTOMATOR_API int get_screen_width(void);
    AUTOMATOR_API int get_screen_height(void);
    AUTOMATOR_API ScreenRegion get_system_tray_region(void);
    AUTOMATOR_API ScreenRegion get_system_tray_region_auto(void);

    /* Функции OpenCV */
    AUTOMATOR_API int opencv_init(void);
    AUTOMATOR_API void opencv_cleanup(void);
    AUTOMATOR_API DetectedRegion detect_system_tray_region_opencv(void);
    AUTOMATOR_API DetectedRegion *find_text_regions_in_tray(ScreenRegion region, int *region_count);
    AUTOMATOR_API DetectedRegion find_keyboard_layout_in_region(ScreenRegion region);

    /* Функции OCR. tessdata_path и tesseract_exe_path могут быть NULL —
       тогда используется автопоиск (см. ocr_init). */
    AUTOMATOR_API int ocr_init(const char *tessdata_path);
    AUTOMATOR_API int ocr_set_tesseract_path(const char *tesseract_exe_path);
    AUTOMATOR_API void ocr_cleanup(void);
    AUTOMATOR_API OCRResult ocr_from_file(const char *image_path, const char *language, const unsigned int psm, const unsigned int dpi);
    AUTOMATOR_API void ocr_result_free(OCRResult *result);

#ifdef __cplusplus
}
#endif

#endif
