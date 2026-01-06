#ifndef OCR_UTILS_H
#define OCR_UTILS_H

#ifdef __cplusplus
extern "C"
{
#endif

    /* Функция определения раскладки по тексту */
    int detect_keyboard_layout_from_text(const char *text);

    /* Функция для получения строкового представления раскладки */
    const char *layout_to_string(int layout);

    /* Функция для получения короткого обозначения раскладки */
    const char *layout_to_short_string(int layout);

#ifdef __cplusplus
}
#endif

#endif /* OCR_UTILS_H */