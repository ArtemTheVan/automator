#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include "ocr.h"

/* Вспомогательные функции */
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

    /* Trim leading spaces */
    char *start = str;
    while (*start == ' ' || *start == '\n' || *start == '\r' || *start == '\t')
    {
        start++;
    }

    /* All spaces? */
    if (*start == '\0')
    {
        str[0] = '\0';
        return str;
    }

    /* Trim trailing spaces */
    char *end = start + strlen(start) - 1;
    while (end > start && (*end == ' ' || *end == '\n' || *end == '\r' || *end == '\t'))
    {
        end--;
    }

    *(end + 1) = '\0';

    /* Move trimmed string to beginning if needed */
    if (start != str)
    {
        memmove(str, start, strlen(start) + 1);
    }

    return str;
}

/* Основные функции управления OCR */
int ocr_init(const char *custom_tessdata_path)
{
    printf("Initializing OCR system...\n");

    /* Проверяем наличие Tesseract в стандартных местах */
    const char *possible_paths[] = {
        "C:\\Program Files\\Tesseract-OCR\\tesseract.exe",
        "C:\\Program Files (x86)\\Tesseract-OCR\\tesseract.exe",
        "C:\\Tesseract-OCR\\tesseract.exe",
        NULL};

    int found = 0;
    char tesseract_path[MAX_PATH] = "";

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

    printf("OCR initialized successfully\n");
    printf("  Tesseract: %s\n", tesseract_path);

    return OCR_SUCCESS;
}

void ocr_cleanup(void)
{
    printf("OCR system cleaned up\n");
}

void ocr_result_free(OCRResult *result)
{
    if (result && result->text)
    {
        free(result->text);
        result->text = NULL;
        result->text_length = 0;
    }
}

/* Преобразует UTF-8 в CP1251 */
static char *convert_utf8_to_cp1251(const char *utf8_str)
{
    if (!utf8_str)
        return NULL;

    size_t len = strlen(utf8_str);
    char *result = (char *)malloc(len * 2 + 1); /* Максимум в 2 раза больше */
    if (!result)
        return NULL;

    int j = 0;
    for (int i = 0; utf8_str[i]; i++)
    {
        unsigned char c = (unsigned char)utf8_str[i];

        /* Простая конвертация наиболее распространенных символов */
        if (c <= 0x7F)
        {
            result[j++] = c;
        }
        else if (c == 0xD0 && utf8_str[i + 1])
        {
            unsigned char next = (unsigned char)utf8_str[i + 1];
            if (next >= 0x90 && next <= 0xBF)
            { /* А-п */
                result[j++] = next + 0x40;
                i++;
            }
            else if (next == 0x81)
            { /* Ё */
                result[j++] = 0xA8;
                i++;
            }
        }
        else if (c == 0xD1 && utf8_str[i + 1])
        {
            unsigned char next = (unsigned char)utf8_str[i + 1];
            if (next >= 0x80 && next <= 0x8F)
            { /* р-я */
                result[j++] = next + 0x70;
                i++;
            }
            else if (next == 0x91)
            { /* ё */
                result[j++] = 0xB8;
                i++;
            }
        }
        else
        {
            /* Оставляем как есть */
            result[j++] = c;
        }
    }
    result[j] = '\0';

    return result;
}

/* Очищает текст OCR */
static void clean_ocr_text(char *text)
{
    char cleaned[1024] = {0};
    int j = 0;

    for (int i = 0; text[i] && j < 1023; i++)
    {
        unsigned char c = (unsigned char)text[i];

        /* Латинские буквы - в верхний регистр */
        if (c >= 'a' && c <= 'z')
        {
            cleaned[j++] = c - 32; /* В верхний регистр */
        }
        else if (c >= 'A' && c <= 'Z')
        {
            cleaned[j++] = c;
        }
        /* Кириллица в CP1251 (А-Я, а-я, Ёё) */
        else if ((c >= 0xC0 && c <= 0xFF) || c == 0xA8 || c == 0xB8)
        {
            /* Оставляем как есть, но можно преобразовать в верхний регистр */
            if (c >= 0xE0 && c <= 0xFF)
            {                            /* а-я */
                cleaned[j++] = c - 0x20; /* В верхний регистр */
            }
            else if (c == 0xB8)
            {                        /* ё */
                cleaned[j++] = 0xA8; /* Ё */
            }
            else
            {
                cleaned[j++] = c;
            }
        }
        /* Цифры */
        else if (c >= '0' && c <= '9')
        {
            cleaned[j++] = c;
        }
        /* Пробелы и дефисы */
        else if (c == ' ' || c == '-')
        {
            cleaned[j++] = c;
        }
    }

    cleaned[j] = '\0';
    strcpy(text, cleaned);
}

static int run_tesseract_simple(const char *image_path, const char *output_file, const char *language, const unsigned int psm)
{
    char command[4096];

    /* Используем путь по умолчанию */
    sprintf(command, "cmd /c \"\"C:\\Program Files\\Tesseract-OCR\\tesseract.exe\" \"%s\" \"%s\" -l %s --psm %u\"",
            image_path, output_file, language, psm);

    printf("Executing: %s\n", command);

    int result = system(command);

    if (result != 0)
    {
        printf("Tesseract returned error code: %d\n", result);
    }

    return result;
}

/* Проверяет, содержит ли текст странные символы */
static int contains_weird_chars(const char *text)
{
    for (int i = 0; text[i]; i++)
    {
        unsigned char c = (unsigned char)text[i];
        /* Проверяем на символы, которые выглядят как поврежденная кириллица */
        if (c >= 0xD0 && c <= 0xDF && text[i + 1])
        {
            /* Это может быть UTF-8, но если следующий символ тоже странный... */
            unsigned char next = (unsigned char)text[i + 1];
            if (next >= 0x80 && next <= 0xBF)
            {
                return 1;
            }
        }
    }
    return 0;
}

/* Исправляет кодировку текста */
static void fix_text_encoding(char *text)
{
    /* Простая попытка исправить UTF-8 проблемы */
    char fixed[1024] = {0};
    int j = 0;

    for (int i = 0; text[i] && j < 1023; i++)
    {
        unsigned char c = (unsigned char)text[i];

        /* Пытаемся преобразовать UTF-8 последовательности */
        if (c >= 0xD0 && c <= 0xDF && text[i + 1])
        {
            unsigned char next = (unsigned char)text[i + 1];
            if (c == 0xD1 && next == 0x81)
            {                      /* 'Ё' */
                fixed[j++] = 0xCB; /* В CP1251 */
                i++;               /* Пропускаем следующий байт */
            }
            else if (c == 0xD0 && next >= 0x90 && next <= 0xBF)
            {                             /* 'А'-'п' */
                fixed[j++] = next + 0x40; /* Преобразуем в CP1251 */
                i++;
            }
            else if (c == 0xD1 && next >= 0x80 && next <= 0x8F)
            {                             /* 'р'-'я' */
                fixed[j++] = next + 0x70; /* Преобразуем в CP1251 */
                i++;
            }
            else
            {
                fixed[j++] = c;
            }
        }
        else if (c >= 0x80 && c <= 0xFF)
        {
            /* Оставляем как есть - возможно, это уже CP1251 */
            fixed[j++] = c;
        }
        else if (c >= 32 && c <= 126)
        {
            /* Печатные ASCII символы */
            fixed[j++] = c;
        }
        /* Игнорируем остальное */
    }

    fixed[j] = '\0';
    strcpy(text, fixed);
}

/* Рассчитывает уверенность распознавания */
static float calculate_confidence(const char *text)
{
    int letter_count = 0;
    int total_chars = 0;

    for (int i = 0; text[i]; i++)
    {
        unsigned char c = (unsigned char)text[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= 0xC0 && c <= 0xFF)) /* Кириллица в CP1251 */
        {
            letter_count++;
        }
        if (c >= 32) /* Не управляющие символы */
        {
            total_chars++;
        }
    }

    if (total_chars == 0)
        return 0.0f;

    float ratio = (float)letter_count / total_chars;

    /* Базовая уверенность */
    float confidence = 0.5f + ratio * 0.5f;

    /* Дополнительные бонусы за ключевые слова */
    if (strstr(text, "ENG") || strstr(text, "EN") || strstr(text, "US"))
        confidence += 0.2f;
    if (strstr(text, "RUS") || strstr(text, "RU") ||
        strstr(text, "РУС") || strstr(text, "РУ"))
        confidence += 0.2f;

    return confidence > 1.0f ? 1.0f : confidence;
}

/* Основная функция распознавания через Tesseract */
OCRResult ocr_from_file(const char *image_path, const char *language, const unsigned int psm, const unsigned int dpi)
{
    OCRResult result = {0};

    /* Проверяем существование файла */
    if (!is_file_exists(image_path))
    {
        result.error_code = OCR_ERROR_FILE_NOT_FOUND;
        return result;
    }

    /* Генерируем временный файл для результата */
    char output_file[MAX_PATH];
    char temp_output_file[MAX_PATH];
    GetTempPath(MAX_PATH, temp_output_file);
    snprintf(temp_output_file, MAX_PATH, "%socr_result_%lu", temp_output_file, GetCurrentProcessId());
    strcpy(output_file, temp_output_file);

    /* Если язык не указан, используем английский по умолчанию */
    const char *lang = language ? language : "eng";

    /* Для маленьких изображений используем специальные настройки */
    unsigned int actual_psm = psm;
    unsigned int actual_dpi = dpi;

    /* Если PSM не указан, выбираем оптимальный для маленького текста */
    if (psm == 0)
    {
        actual_psm = 7; /* PSM 7: Одна строка текста */
    }

    /* Если DPI не указан, используем 300 для небольших изображений */
    if (dpi == 0)
    {
        actual_dpi = 300;
    }

    /* Дополнительные параметры для улучшения распознавания */
    char command[4096];

    /* УПРОЩЕННАЯ команда без whitelist - он может мешать распознаванию */
    sprintf(command, "cmd /c \"\"C:\\Program Files\\Tesseract-OCR\\tesseract.exe\" \"%s\" \"%s\" -l %s --psm %u --dpi %u --oem 1\"",
            image_path, output_file, lang, actual_psm, actual_dpi);

    printf("Executing: %s\n", command);

    int ret = system(command);

    if (ret != 0)
    {
        printf("Tesseract returned error code: %d\n", ret);

        /* Попробуем без указания DPI */
        sprintf(command, "cmd /c \"\"C:\\Program Files\\Tesseract-OCR\\tesseract.exe\" \"%s\" \"%s\" -l %s --psm %u\"",
                image_path, output_file, lang, actual_psm);
        printf("Trying without DPI: %s\n", command);

        ret = system(command);

        if (ret != 0)
        {
            result.error_code = OCR_ERROR_INIT_FAILED;
            return result;
        }
    }

    /* Читаем результат из файла */
    char result_file[MAX_PATH];
    char temp_result_file[MAX_PATH];
    snprintf(temp_result_file, MAX_PATH, "%s.txt", output_file);
    strcpy(result_file, temp_result_file);

    FILE *fp = fopen(result_file, "rb"); /* Открываем в бинарном режиме */
    if (!fp)
    {
        result.error_code = OCR_ERROR_FILE_NOT_FOUND;
        return result;
    }

    /* Определяем размер файла */
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

    /* Читаем содержимое в UTF-8 */
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

    /* Удаляем временный файл */
    remove(result_file);

    /* Очищаем и обрезаем текст */
    if (result.text_length > 0)
    {
        trim_string(result.text);
        result.text_length = (int)strlen(result.text);

        /* Преобразуем текст в Windows-1251 для простоты анализа */
        char *converted = convert_utf8_to_cp1251(result.text);
        if (converted)
        {
            free(result.text);
            result.text = converted;
            result.text_length = (int)strlen(result.text);
        }

        /* Удаляем все не-буквенные символы и приводим к верхнему регистру */
        clean_ocr_text(result.text);
    }

    if (result.text_length > 0)
    {
        /* Рассчитываем уверенность на основе длины текста и содержания букв */
        result.confidence = calculate_confidence(result.text);
        result.error_code = OCR_SUCCESS;
        printf("OCR recognized: '%s' (confidence: %.2f)\n", result.text, result.confidence);
    }
    else
    {
        free(result.text);
        result.text = NULL;
        result.error_code = OCR_ERROR_NO_TEXT;
    }

    return result;
}