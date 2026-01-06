#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "ocr_utils.h"

/* Улучшенная функция определения раскладки по тексту */
int detect_keyboard_layout_from_text(const char *text)
{
    if (!text || strlen(text) == 0)
    {
        return -1; // Ошибка: нет текста
    }

    // Создаем копию текста в нижнем регистре для сравнения
    char lower_text[256];
    strncpy(lower_text, text, sizeof(lower_text) - 1);
    lower_text[sizeof(lower_text) - 1] = '\0';

    for (int i = 0; lower_text[i]; i++)
    {
        lower_text[i] = tolower(lower_text[i]);
    }

    printf("OCR распознал: '%s'\n", text);
    printf("В нижнем регистре: '%s'\n", lower_text);

    // Расширенный список индикаторов
    const char *russian_indicators[] = {
        "rus", "ru", "рус", "ру", "pyc", "pус", // pyc - частая ошибка OCR
        "russian", "язык", "language", "ruski",
        NULL};

    const char *english_indicators[] = {
        "eng", "en", "us", "англ", "english",
        "американ", "английск", "united", "states",
        NULL};

    // Проверяем русские индикаторы
    for (int i = 0; russian_indicators[i] != NULL; i++)
    {
        if (strstr(lower_text, russian_indicators[i]))
        {
            printf("Найден русский индикатор: '%s'\n", russian_indicators[i]);
            return 1;
        }
    }

    // Проверяем английские индикаторы
    for (int i = 0; english_indicators[i] != NULL; i++)
    {
        if (strstr(lower_text, english_indicators[i]))
        {
            printf("Найден английский индикатор: '%s'\n", english_indicators[i]);
            return 0;
        }
    }

    // Ищем отдельные символы RU/ENG в разных форматах
    if (strstr(lower_text, " ru") || strstr(lower_text, "ru ") ||
        strstr(text, "RU") || strstr(text, "RUS"))
    {
        printf("Найдено RU/RUS в тексте\n");
        return 1;
    }

    if (strstr(lower_text, " en") || strstr(lower_text, "en ") ||
        strstr(text, "EN") || strstr(text, "ENG"))
    {
        printf("Найдено EN/ENG в тексте\n");
        return 0;
    }

    // Ищем русские буквы (кириллицу)
    const char *russian_chars = "абвгдеёжзийклмнопрстуфхцчшщъыьэюя";
    for (int i = 0; russian_chars[i] != '\0'; i++)
    {
        if (strchr(text, russian_chars[i]) || strchr(text, toupper(russian_chars[i])))
        {
            printf("Найдена русская буква: %c\n", russian_chars[i]);
            return 1;
        }
    }

    return -2; // Не удалось определить
}

/* Функция для получения строкового представления раскладки */
/* Get layout string representation */
const char* layout_to_string(int layout)
{
    switch (layout)
    {
    case 0:
        return "ENG (English)";
    case 1:
        return "RUS (Russian)";
    case -1:
        return "ERROR (Failed to detect)";
    case -2:
        return "UNKNOWN (Cannot determine)";
    default:
        return "UNKNOWN";
    }
}

/* Функция для получения короткого обозначения раскладки */
const char *layout_to_short_string(int layout)
{
    switch (layout)
    {
    case 0:
        return "ENG";
    case 1:
        return "RUS";
    case -1:
        return "ERR";
    case -2:
        return "UNK";
    default:
        return "???";
    }
}