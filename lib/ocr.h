#ifndef OCR_H_
#define OCR_H_

#include <windows.h>

/* Макросы для экспорта функций из DLL */
#ifdef BUILDING_AUTOMATOR_DLL /* ИЗМЕНЕНО с BUILDING_OCR_DLL */
#define OCR_API __declspec(dllexport)
#else
#define OCR_API __declspec(dllimport)
#endif

/* Коды ошибок */
#define OCR_SUCCESS 0
#define OCR_ERROR_INIT_FAILED -1
#define OCR_ERROR_NO_TEXT -2
#define OCR_ERROR_LOW_CONFIDENCE -3
#define OCR_ERROR_CAPTURE_FAILED -4
#define OCR_ERROR_FILE_NOT_FOUND -5
#define OCR_ERROR_INVALID_PARAMS -6
#define OCR_ERROR_TESSERACT_NOT_FOUND -7
#define OCR_ERROR_IMAGE_EMPTY -8

/* Структура для результатов OCR */
typedef struct
{
    char *text;
    float confidence;
    int text_length;
    int error_code;
} OCRResult;

/* Функции для работы с OCR */
OCR_API int ocr_init(const char *tessdata_path);
OCR_API void ocr_cleanup(void);
OCR_API OCRResult ocr_from_file(const char *image_path, const char *language, const unsigned int psm, const unsigned int dpi);
OCR_API void ocr_result_free(OCRResult *result);

#endif