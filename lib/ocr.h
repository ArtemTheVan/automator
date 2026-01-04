#ifndef OCR_H_
#define OCR_H_

#include <windows.h>
#include <stdio.h>
#include <string.h>

/* Структура для результатов OCR */
typedef struct
{
    char *text;       // Распознанный текст
    float confidence; // Уровень уверенности
    int text_length;  // Длина текста
    int error_code;   // Код ошибки (0 - успех)
} OCRResult;

/* Инициализация/деинициализация OCR системы */
int ocr_init(const char *tessdata_path);
void ocr_cleanup();
int ocr_is_initialized();

/* Основные функции распознавания текста */
OCRResult ocr_from_bitmap(HBITMAP bitmap, const char *language);
OCRResult ocr_from_file(const char *image_path, const char *language);
OCRResult ocr_from_screen_region(int x, int y, int width, int height, const char *language);

/* Утилиты для работы с результатами OCR */
void ocr_result_free(OCRResult *result);
char *ocr_result_to_string(OCRResult result);
void ocr_result_print(OCRResult result, const char *label);
OCRResult ocr_create_empty_result();
OCRResult ocr_create_error_result(int error_code);

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

#endif