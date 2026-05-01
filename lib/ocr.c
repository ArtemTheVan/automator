#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "ocr.h"
#include "log.h"

/*
 * OCR-модуль реализован поверх вызова tesseract.exe как внешнего процесса.
 * Это позволяет работать без линковки против libtesseract/leptonica, но
 * имеет накладные расходы на запуск процесса для каждого вызова.
 *
 * TODO: миграция на C API (libtesseract) даст значительный выигрыш по
 * скорости и устранит зависимость от внешнего exe.
 */

/* Конфигурируемые пути; могут быть установлены через ocr_set_tesseract_path()
   или переменные окружения TESSERACT_EXE / TESSDATA_PREFIX. */
static char g_tesseract_exe[MAX_PATH] = {0};
static char g_tessdata_prefix[MAX_PATH] = {0};

/* Список стандартных мест установки Tesseract под Windows. */
static const char *DEFAULT_TESSERACT_PATHS[] = {
    "C:\\Program Files\\Tesseract-OCR\\tesseract.exe",
    "C:\\Program Files (x86)\\Tesseract-OCR\\tesseract.exe",
    "C:\\Tesseract-OCR\\tesseract.exe",
    NULL
};

/* ===== Вспомогательные функции ===== */

static int file_exists(const char *path)
{
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

static void copy_string(char *dst, size_t dst_size, const char *src)
{
    if (!dst || dst_size == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    size_t n = strlen(src);
    if (n >= dst_size) n = dst_size - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

static char *trim_string(char *str)
{
    if (!str) return NULL;
    char *start = str;
    while (*start == ' ' || *start == '\n' || *start == '\r' || *start == '\t') start++;
    if (*start == '\0') { str[0] = '\0'; return str; }
    char *end = start + strlen(start) - 1;
    while (end > start && (*end == ' ' || *end == '\n' || *end == '\r' || *end == '\t')) end--;
    *(end + 1) = '\0';
    if (start != str) memmove(str, start, strlen(start) + 1);
    return str;
}

/* Поиск установленного tesseract.exe.
   Приоритет: явная установка через ocr_set_tesseract_path > env > стандартные пути. */
static int discover_tesseract_path(char *out, size_t out_size)
{
    if (g_tesseract_exe[0] && file_exists(g_tesseract_exe)) {
        copy_string(out, out_size, g_tesseract_exe);
        return 1;
    }

    char env_path[MAX_PATH];
    DWORD env_len = GetEnvironmentVariableA("TESSERACT_EXE", env_path, sizeof(env_path));
    if (env_len > 0 && env_len < sizeof(env_path) && file_exists(env_path)) {
        copy_string(out, out_size, env_path);
        return 1;
    }

    for (int i = 0; DEFAULT_TESSERACT_PATHS[i]; i++) {
        if (file_exists(DEFAULT_TESSERACT_PATHS[i])) {
            copy_string(out, out_size, DEFAULT_TESSERACT_PATHS[i]);
            return 1;
        }
    }
    return 0;
}

/*
 * Безопасное экранирование одного аргумента командной строки Windows.
 * Реализация по правилам CommandLineToArgvW (CSRT). Используется только
 * как защитная мера; основная защита от инъекций — CreateProcess с массивом.
 */
static int append_quoted(char *dst, size_t dst_size, size_t pos, const char *arg)
{
    if (pos + 1 >= dst_size) return -1;
    dst[pos++] = '"';

    int backslashes = 0;
    for (const char *p = arg; *p; p++) {
        if (*p == '\\') {
            backslashes++;
        } else if (*p == '"') {
            for (int i = 0; i < backslashes * 2 + 1; i++) {
                if (pos + 1 >= dst_size) return -1;
                dst[pos++] = '\\';
            }
            backslashes = 0;
            if (pos + 1 >= dst_size) return -1;
            dst[pos++] = '"';
        } else {
            for (int i = 0; i < backslashes; i++) {
                if (pos + 1 >= dst_size) return -1;
                dst[pos++] = '\\';
            }
            backslashes = 0;
            if (pos + 1 >= dst_size) return -1;
            dst[pos++] = *p;
        }
    }
    /* Удвоить trailing backslashes перед закрывающей кавычкой. */
    for (int i = 0; i < backslashes * 2; i++) {
        if (pos + 1 >= dst_size) return -1;
        dst[pos++] = '\\';
    }
    if (pos + 1 >= dst_size) return -1;
    dst[pos++] = '"';
    dst[pos] = '\0';
    return (int)pos;
}

/* Запуск Tesseract через CreateProcess вместо system().
   Возвращает 0 при успехе, иначе ненулевое значение. */
static int run_tesseract(const char *exe, const char *image_path, const char *out_base,
                         const char *language, unsigned int psm, unsigned int dpi)
{
    char cmdline[8192];
    size_t pos = 0;
    cmdline[0] = '\0';

    int n = append_quoted(cmdline, sizeof(cmdline), pos, exe);
    if (n < 0) return -1;
    pos = (size_t)n;
    if (pos + 1 >= sizeof(cmdline)) return -1;
    cmdline[pos++] = ' ';

    n = append_quoted(cmdline, sizeof(cmdline), pos, image_path);
    if (n < 0) return -1;
    pos = (size_t)n;
    if (pos + 1 >= sizeof(cmdline)) return -1;
    cmdline[pos++] = ' ';

    n = append_quoted(cmdline, sizeof(cmdline), pos, out_base);
    if (n < 0) return -1;
    pos = (size_t)n;

    int written = snprintf(cmdline + pos, sizeof(cmdline) - pos,
                           " -l %s --psm %u --dpi %u --oem 1",
                           language, psm, dpi);
    if (written < 0 || (size_t)written >= sizeof(cmdline) - pos) return -1;

    LOG_DEBUG("Running tesseract: %s", cmdline);

    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    /* Имитируем установку TESSDATA_PREFIX для дочернего процесса. */
    LPCH parent_env = GetEnvironmentStringsA();
    LPVOID child_env = NULL;
    char *new_env = NULL;
    size_t new_env_size = 0;
    if (g_tessdata_prefix[0]) {
        size_t parent_len = 0;
        const char *p = parent_env;
        while (*p) { size_t l = strlen(p) + 1; parent_len += l; p += l; }
        size_t add_len = strlen("TESSDATA_PREFIX=") + strlen(g_tessdata_prefix) + 1;
        new_env_size = parent_len + add_len + 1;
        new_env = (char *)malloc(new_env_size);
        if (new_env) {
            memcpy(new_env, parent_env, parent_len);
            char *w = new_env + parent_len;
            int wn = snprintf(w, add_len, "TESSDATA_PREFIX=%s", g_tessdata_prefix);
            if (wn >= 0) w[wn] = '\0';
            new_env[parent_len + add_len] = '\0';
            child_env = new_env;
        }
    }

    BOOL ok = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE,
                             CREATE_NO_WINDOW, child_env, NULL, &si, &pi);

    if (parent_env) FreeEnvironmentStringsA(parent_env);

    if (!ok) {
        LOG_ERROR("CreateProcess failed: %lu", GetLastError());
        free(new_env);
        return -1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exit_code = 1;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    free(new_env);

    return (int)exit_code;
}

/* Преобразует UTF-8 → CP1251 через WinAPI (заменяет хрупкую ручную таблицу). */
static char *utf8_to_cp1251(const char *utf8_str)
{
    if (!utf8_str) return NULL;

    int wide_len = MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, NULL, 0);
    if (wide_len <= 0) return NULL;
    wchar_t *wide = (wchar_t *)malloc(wide_len * sizeof(wchar_t));
    if (!wide) return NULL;
    MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, wide, wide_len);

    int cp_len = WideCharToMultiByte(1251, 0, wide, -1, NULL, 0, NULL, NULL);
    if (cp_len <= 0) { free(wide); return NULL; }
    char *out = (char *)malloc(cp_len);
    if (!out) { free(wide); return NULL; }
    WideCharToMultiByte(1251, 0, wide, -1, out, cp_len, NULL, NULL);
    free(wide);
    return out;
}

static void clean_ocr_text(char *text)
{
    if (!text) return;
    char cleaned[4096] = {0};
    size_t j = 0;

    for (size_t i = 0; text[i] && j < sizeof(cleaned) - 1; i++) {
        unsigned char c = (unsigned char)text[i];

        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= 0xC0 && c <= 0xFF) || c == 0xA8 || c == 0xB8 ||
            (c >= '0' && c <= '9'))
        {
            cleaned[j++] = c;
        }
        else if (c == '.' || c == ':' || c == ',' || c == ';' ||
                 c == '-' || c == '_' || c == '/' || c == '\\' ||
                 c == '(' || c == ')' || c == '[' || c == ']' ||
                 c == '{' || c == '}' || c == '<' || c == '>' ||
                 c == '!' || c == '?' || c == '@' || c == '#' ||
                 c == '$' || c == '%' || c == '^' || c == '&' ||
                 c == '*' || c == '+' || c == '=' || c == '|' ||
                 c == '~' || c == '`' || c == '\'' || c == '"' ||
                 c == ' ' || c == '\t')
        {
            cleaned[j++] = c;
        }
        else if (c >= 32) {
            cleaned[j++] = c;
        }
        /* < 32 — пропускаем управляющие */
    }

    cleaned[j] = '\0';
    if (strcmp(text, cleaned) != 0) {
        memcpy(text, cleaned, j + 1);
    }
}

static float calculate_confidence(const char *text)
{
    int meaningful = 0;
    int total = 0;
    for (int i = 0; text[i]; i++) {
        unsigned char c = (unsigned char)text[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            (c >= 0xC0 && c <= 0xFF) ||
            c == '.' || c == ':' || c == ',' || c == ';' ||
            c == '-' || c == '_' || c == '/' || c == '\\' ||
            c == '(' || c == ')' || c == '[' || c == ']' ||
            c == '{' || c == '}' || c == ' ' || c == '\t')
        {
            meaningful++;
        }
        if (c >= 32) total++;
    }
    if (total == 0) return 0.0f;
    float ratio = (float)meaningful / total;
    float confidence = 0.3f + ratio * 0.7f;
    return confidence > 1.0f ? 1.0f : confidence;
}

/* ===== Публичный API ===== */

int ocr_set_tesseract_path(const char *exe_path)
{
    if (!exe_path || !exe_path[0]) {
        g_tesseract_exe[0] = '\0';
        return OCR_SUCCESS;
    }
    if (!file_exists(exe_path)) {
        LOG_ERROR("Tesseract not found at: %s", exe_path);
        return OCR_ERROR_TESSERACT_NOT_FOUND;
    }
    copy_string(g_tesseract_exe, sizeof(g_tesseract_exe), exe_path);
    LOG_INFO("Tesseract executable set to: %s", g_tesseract_exe);
    return OCR_SUCCESS;
}

int ocr_init(const char *custom_tessdata_path)
{
    LOG_INFO("Initializing OCR system...");

    /* Сохраняем явно переданный путь к tessdata. */
    if (custom_tessdata_path && custom_tessdata_path[0]) {
        copy_string(g_tessdata_prefix, sizeof(g_tessdata_prefix), custom_tessdata_path);
        LOG_INFO("TESSDATA_PREFIX set to: %s", g_tessdata_prefix);
    } else {
        char env_path[MAX_PATH];
        DWORD len = GetEnvironmentVariableA("TESSDATA_PREFIX", env_path, sizeof(env_path));
        if (len > 0 && len < sizeof(env_path)) {
            copy_string(g_tessdata_prefix, sizeof(g_tessdata_prefix), env_path);
        }
    }

    char tesseract[MAX_PATH];
    if (!discover_tesseract_path(tesseract, sizeof(tesseract))) {
        LOG_ERROR("Tesseract OCR not found. Install from "
                  "https://github.com/UB-Mannheim/tesseract/wiki "
                  "or call ocr_set_tesseract_path()");
        return OCR_ERROR_TESSERACT_NOT_FOUND;
    }

    LOG_INFO("Tesseract: %s", tesseract);
    return OCR_SUCCESS;
}

void ocr_cleanup(void)
{
    LOG_DEBUG("OCR system cleaned up");
}

void ocr_result_free(OCRResult *result)
{
    if (result && result->text) {
        free(result->text);
        result->text = NULL;
        result->text_length = 0;
    }
}

OCRResult ocr_from_file(const char *image_path, const char *language,
                        const unsigned int psm, const unsigned int dpi)
{
    OCRResult result = {0};

    if (!image_path || !file_exists(image_path)) {
        result.error_code = OCR_ERROR_FILE_NOT_FOUND;
        return result;
    }

    char tesseract[MAX_PATH];
    if (!discover_tesseract_path(tesseract, sizeof(tesseract))) {
        result.error_code = OCR_ERROR_TESSERACT_NOT_FOUND;
        return result;
    }

    /* Уникальный временный файл результата. */
    char temp_dir[MAX_PATH];
    GetTempPathA(MAX_PATH, temp_dir);
    char output_base[MAX_PATH];
    int n = snprintf(output_base, sizeof(output_base),
                     "%socr_result_%lu_%lu",
                     temp_dir, GetCurrentProcessId(), GetTickCount());
    if (n < 0 || (size_t)n >= sizeof(output_base)) {
        result.error_code = OCR_ERROR_INVALID_PARAMS;
        return result;
    }

    const char *lang = (language && language[0]) ? language : "eng";
    unsigned int actual_psm = psm == 0 ? 7 : psm;
    unsigned int actual_dpi = dpi == 0 ? 300 : dpi;

    int rc = run_tesseract(tesseract, image_path, output_base,
                           lang, actual_psm, actual_dpi);
    if (rc != 0) {
        LOG_ERROR("Tesseract returned error code: %d", rc);
        result.error_code = OCR_ERROR_INIT_FAILED;
        return result;
    }

    char result_file[MAX_PATH];
    n = snprintf(result_file, sizeof(result_file), "%s.txt", output_base);
    if (n < 0 || (size_t)n >= sizeof(result_file)) {
        result.error_code = OCR_ERROR_INVALID_PARAMS;
        return result;
    }

    FILE *fp = fopen(result_file, "rb");
    if (!fp) {
        result.error_code = OCR_ERROR_FILE_NOT_FOUND;
        return result;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size <= 0) {
        fclose(fp);
        remove(result_file);
        result.error_code = OCR_ERROR_NO_TEXT;
        return result;
    }

    result.text = (char *)malloc(file_size + 1);
    if (!result.text) {
        fclose(fp);
        remove(result_file);
        result.error_code = OCR_ERROR_NO_TEXT;
        return result;
    }

    size_t read_bytes = fread(result.text, 1, file_size, fp);
    result.text[read_bytes] = '\0';
    result.text_length = (int)read_bytes;
    fclose(fp);
    remove(result_file);

    if (result.text_length > 0) {
        trim_string(result.text);
        result.text_length = (int)strlen(result.text);

        char *converted = utf8_to_cp1251(result.text);
        if (converted) {
            free(result.text);
            result.text = converted;
            result.text_length = (int)strlen(result.text);
        }

        clean_ocr_text(result.text);
        result.text_length = (int)strlen(result.text);
    }

    if (result.text_length > 0) {
        result.confidence = calculate_confidence(result.text);
        result.error_code = OCR_SUCCESS;
        LOG_DEBUG("OCR recognized: '%s' (confidence: %.2f)",
                  result.text, result.confidence);
    } else {
        free(result.text);
        result.text = NULL;
        result.error_code = OCR_ERROR_NO_TEXT;
    }

    return result;
}
