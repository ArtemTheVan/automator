#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include "ocr.h"
#include "screen.h"

/* Внутренние переменные */
static int ocr_initialized = 0;
static char tesseract_path[MAX_PATH] = "C:\\Program Files\\Tesseract-OCR\\tesseract.exe";
static char tessdata_path[MAX_PATH] = "C:\\Program Files\\Tesseract-OCR\\tessdata";

/* Вспомогательные функции */
static char *generate_temp_filename(const char *extension)
{
    static char filename[MAX_PATH];
    char temp_path[MAX_PATH];

    GetTempPath(MAX_PATH, temp_path);
    srand((unsigned int)time(NULL));
    sprintf(filename, "%socr_temp_%d.%s", temp_path, rand() % 10000, extension);

    return filename;
}

static int is_file_exists(const char *filename)
{
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile(filename, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return 0;
    }
    FindClose(hFind);
    return 1;
}

static char *trim_string(char *str)
{
    if (!str)
        return NULL;

    // Trim leading spaces
    char *start = str;
    while (*start == ' ' || *start == '\n' || *start == '\r' || *start == '\t')
    {
        start++;
    }

    // All spaces?
    if (*start == '\0')
    {
        str[0] = '\0';
        return str;
    }

    // Trim trailing spaces
    char *end = start + strlen(start) - 1;
    while (end > start && (*end == ' ' || *end == '\n' || *end == '\r' || *end == '\t'))
    {
        end--;
    }

    *(end + 1) = '\0';

    // Move trimmed string to beginning if needed
    if (start != str)
    {
        memmove(str, start, strlen(start) + 1);
    }

    return str;
}

/* Основные функции управления OCR */
int ocr_init(const char *custom_tessdata_path)
{
    if (ocr_initialized)
    {
        return OCR_SUCCESS;
    }

    printf("Initializing OCR system...\n");

    // Проверяем наличие Tesseract в стандартных местах
    const char *possible_paths[] = {
        "C:\\Program Files\\Tesseract-OCR\\tesseract.exe",
        "C:\\Program Files (x86)\\Tesseract-OCR\\tesseract.exe",
        "C:\\Tesseract-OCR\\tesseract.exe",
        NULL};

    int found = 0;
    for (int i = 0; possible_paths[i]; i++)
    {
        if (is_file_exists(possible_paths[i]))
        {
            strcpy(tesseract_path, possible_paths[i]);
            found = 1;
            break;
        }
    }

    if (!found)
    {
        fprintf(stderr, "Tesseract OCR not found. Please install from https://github.com/UB-Mannheim/tesseract/wiki\n");
        return OCR_ERROR_TESSERACT_NOT_FOUND;
    }

    // Устанавливаем путь к tessdata
    if (custom_tessdata_path)
    {
        strcpy(tessdata_path, custom_tessdata_path);
    }
    else
    {
        // Автоматически определяем путь к tessdata
        char *last_slash = strrchr(tesseract_path, '\\');
        if (last_slash)
        {
            *last_slash = '\0';
            sprintf(tessdata_path, "%s\\tessdata", tesseract_path);
            *last_slash = '\\';
        }
    }

    // Проверяем наличие tessdata
    if (!is_file_exists(tessdata_path))
    {
        printf("Warning: tessdata directory not found at %s\n", tessdata_path);
    }

    ocr_initialized = 1;
    printf("OCR initialized successfully\n");
    printf("  Tesseract: %s\n", tesseract_path);
    printf("  Tessdata: %s\n", tessdata_path);

    return OCR_SUCCESS;
}

void ocr_cleanup()
{
    ocr_initialized = 0;
    printf("OCR system cleaned up\n");
}

int ocr_is_initialized()
{
    return ocr_initialized;
}

/* Функции создания результатов */
OCRResult ocr_create_empty_result()
{
    OCRResult result = {0};
    return result;
}

OCRResult ocr_create_error_result(int error_code)
{
    OCRResult result = {0};
    result.error_code = error_code;
    return result;
}

/* Вспомогательные функции для работы с результатами */
void ocr_result_free(OCRResult *result)
{
    if (result && result->text)
    {
        free(result->text);
        result->text = NULL;
        result->text_length = 0;
    }
}

char *ocr_result_to_string(OCRResult result)
{
    if (result.text && result.error_code == OCR_SUCCESS)
    {
        return strdup(result.text);
    }
    return NULL;
}

void ocr_result_print(OCRResult result, const char *label)
{
    if (label)
    {
        printf("%s: ", label);
    }

    if (result.error_code != OCR_SUCCESS)
    {
        printf("OCR error %d", result.error_code);

        switch (result.error_code)
        {
        case OCR_ERROR_INIT_FAILED:
            printf(" (Initialization failed)");
            break;
        case OCR_ERROR_NO_TEXT:
            printf(" (No text found)");
            break;
        case OCR_ERROR_FILE_NOT_FOUND:
            printf(" (File not found)");
            break;
        case OCR_ERROR_TESSERACT_NOT_FOUND:
            printf(" (Tesseract not found)");
            break;
        }
        printf("\n");
        return;
    }

    if (result.text && result.text_length > 0)
    {
        printf("Text (%d chars, confidence: %.1f%%): '%s'\n",
               result.text_length, result.confidence * 100, result.text);
    }
    else
    {
        printf("Empty result\n");
    }
}

/* Основная функция распознавания через Tesseract */
static OCRResult run_tesseract_ocr(const char *image_path, const char *language)
{
    OCRResult result = ocr_create_empty_result();

    if (!ocr_initialized)
    {
        if (ocr_init(NULL) != OCR_SUCCESS)
        {
            result.error_code = OCR_ERROR_INIT_FAILED;
            return result;
        }
    }

    // Проверяем существование файла
    if (!is_file_exists(image_path))
    {
        result.error_code = OCR_ERROR_FILE_NOT_FOUND;
        return result;
    }

    // Генерируем временный файл для результата
    char output_file[MAX_PATH];
    char command[2048];

    GetTempPath(MAX_PATH, output_file);
    sprintf(output_file, "%socr_result_%d", output_file, GetCurrentProcessId());

    // Если язык не указан, используем английский по умолчанию
    const char *lang = language ? language : "eng";

    // Формируем команду Tesseract с указанием пути к tessdata
    sprintf(command, "\"%s\" \"%s\" \"%s\" -l %s --psm 6 --tessdata-dir \"%s\"",
            tesseract_path, image_path, output_file, lang, tessdata_path);

    // Запускаем Tesseract
    int ret = system(command);
    if (ret != 0)
    {
        result.error_code = OCR_ERROR_INIT_FAILED;
        return result;
    }

    // Читаем результат из файла
    char result_file[MAX_PATH];
    sprintf(result_file, "%s.txt", output_file);

    FILE *fp = fopen(result_file, "r");
    if (!fp)
    {
        result.error_code = OCR_ERROR_FILE_NOT_FOUND;
        return result;
    }

    // Определяем размер файла
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size <= 0)
    {
        fclose(fp);
        remove(result_file);
        result.error_code = OCR_ERROR_NO_TEXT;
        return result;
    }

    // Читаем содержимое
    result.text = (char *)malloc(file_size + 1);
    if (!result.text)
    {
        fclose(fp);
        remove(result_file);
        result.error_code = OCR_ERROR_NO_TEXT;
        return result;
    }

    size_t read_bytes = fread(result.text, 1, file_size, fp);
    result.text[read_bytes] = '\0';
    result.text_length = (int)read_bytes;

    fclose(fp);

    // Удаляем временный файл
    remove(result_file);

    // Очищаем и обрезаем текст
    if (result.text_length > 0)
    {
        trim_string(result.text);
        result.text_length = (int)strlen(result.text);
    }

    if (result.text_length > 0)
    {
        // В реальном проекте нужно парсить уверенность из вывода Tesseract
        result.confidence = 0.85f;
        result.error_code = OCR_SUCCESS;
    }
    else
    {
        free(result.text);
        result.text = NULL;
        result.error_code = OCR_ERROR_NO_TEXT;
    }

    return result;
}

/* Основные OCR функции */
OCRResult ocr_from_file(const char *image_path, const char *language)
{
    return run_tesseract_ocr(image_path, language);
}

OCRResult ocr_from_bitmap(HBITMAP bitmap, const char *language)
{
    if (!bitmap)
    {
        return ocr_create_error_result(OCR_ERROR_INVALID_PARAMS);
    }

    // Сохраняем bitmap во временный файл
    char *temp_file = generate_temp_filename("bmp");
    if (!save_bitmap_to_file(bitmap, temp_file))
    {
        return ocr_create_error_result(OCR_ERROR_CAPTURE_FAILED);
    }

    // Распознаем текст
    OCRResult result = run_tesseract_ocr(temp_file, language);

    // Удаляем временный файл
    remove(temp_file);

    return result;
}

OCRResult ocr_from_screen_region(int x, int y, int width, int height, const char *language)
{
    if (width <= 0 || height <= 0)
    {
        return ocr_create_error_result(OCR_ERROR_INVALID_PARAMS);
    }

    // Захватываем область в bitmap
    HBITMAP bitmap = capture_to_bitmap(x, y, width, height);
    if (!bitmap)
    {
        return ocr_create_error_result(OCR_ERROR_CAPTURE_FAILED);
    }

    // Распознаем текст
    OCRResult result = ocr_from_bitmap(bitmap, language);

    // Освобождаем ресурсы
    DeleteObject(bitmap);

    return result;
}