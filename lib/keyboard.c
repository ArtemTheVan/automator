#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "keyboard.h"
#include "log.h"

/* Задаётся в init.c, переопределяется через set_keystroke_delay_ms(). */
extern int g_keystroke_delay_ms;

/*
 * Конвертирует один UTF-8 символ из позиции *p в кодпойнт Unicode.
 * Возвращает количество прочитанных байт или 0 при ошибке.
 */
static int utf8_to_codepoint(const char *p, unsigned int *out)
{
    unsigned char c = (unsigned char)p[0];
    if (c < 0x80) {
        *out = c;
        return 1;
    }
    if ((c & 0xE0) == 0xC0) {
        if ((p[1] & 0xC0) != 0x80) return 0;
        *out = ((c & 0x1F) << 6) | (p[1] & 0x3F);
        return 2;
    }
    if ((c & 0xF0) == 0xE0) {
        if ((p[1] & 0xC0) != 0x80 || (p[2] & 0xC0) != 0x80) return 0;
        *out = ((c & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
        return 3;
    }
    if ((c & 0xF8) == 0xF0) {
        if ((p[1] & 0xC0) != 0x80 || (p[2] & 0xC0) != 0x80 || (p[3] & 0xC0) != 0x80) return 0;
        *out = ((c & 0x07) << 18) | ((p[1] & 0x3F) << 12)
             | ((p[2] & 0x3F) << 6) | (p[3] & 0x3F);
        return 4;
    }
    return 0;
}

/*
 * Отправляет один UTF-16 code unit как unicode-keystroke (down + up).
 * Использование KEYEVENTF_UNICODE избавляет от необходимости знать раскладку
 * и работает для произвольных символов, включая кириллицу и эмодзи.
 */
static void send_unicode_unit(WORD code_unit)
{
    INPUT inputs[2] = {0};

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = 0;
    inputs[0].ki.wScan = code_unit;
    inputs[0].ki.dwFlags = KEYEVENTF_UNICODE;

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = 0;
    inputs[1].ki.wScan = code_unit;
    inputs[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

    SendInput(2, inputs, sizeof(INPUT));
}

void simulate_keystroke(const char *text)
{
    if (!text) return;
    LOG_DEBUG("Simulating keystrokes for: %s", text);

    const char *p = text;
    while (*p) {
        unsigned int cp = 0;
        int consumed = utf8_to_codepoint(p, &cp);
        if (consumed == 0) {
            LOG_WARN("Invalid UTF-8 byte 0x%02x — skipping", (unsigned char)*p);
            p++;
            continue;
        }
        p += consumed;

        /* BMP (≤ U+FFFF) — один code unit; иначе суррогатная пара. */
        if (cp <= 0xFFFF) {
            send_unicode_unit((WORD)cp);
        } else {
            unsigned int v = cp - 0x10000;
            WORD high = (WORD)(0xD800 + (v >> 10));
            WORD low  = (WORD)(0xDC00 + (v & 0x3FF));
            send_unicode_unit(high);
            send_unicode_unit(low);
        }

        if (g_keystroke_delay_ms > 0) {
            Sleep(g_keystroke_delay_ms);
        }
    }
}
