#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "libautomator.h"
#include "ocr_utils.h"

/* OCR language constants */
#define OCR_LANG_ENGLISH "eng"
#define OCR_LANG_RUSSIAN "rus"
#define OCR_LANG_ENGLISH_RUSSIAN "eng+rus"
#define OCR_LANG_DEFAULT OCR_LANG_ENGLISH_RUSSIAN

/* Keyboard layout detection using Windows API */
int detect_layout_windows_api()
{
    HWND hwnd = GetForegroundWindow();
    if (!hwnd)
    {
        printf("No active window found\n");
        return -1;
    }

    DWORD threadId = GetWindowThreadProcessId(hwnd, NULL);
    HKL layout = GetKeyboardLayout(threadId);
    WORD langId = LOWORD(layout);

    printf("Windows API: Keyboard layout code: 0x%04X\n", langId);

    switch (langId)
    {
    case 0x0409:  // English (United States)
    case 0x0809:  // English (United Kingdom)
    case 0x0C09:  // English (Australian)
    case 0x1009:  // English (Canadian)
    case 0x1409:  // English (New Zealand)
    case 0x1809:  // English (Irish)
    case 0x1C09:  // English (South Africa)
        return 0; // ENG

    case 0x0419:  // Russian
    case 0x0819:  // Russian (Moldova)
        return 1; // RUS

    default:
        printf("Unknown layout code: 0x%04X\n", langId);
        return -2;
    }
}

/* Keyboard layout detection using OCR */
int detect_layout_ocr()
{
    printf("\nUsing OCR for layout detection...\n");

    if (ocr_init(NULL) != OCR_SUCCESS)
    {
        printf("OCR initialization failed\n");
        return -1;
    }

    ScreenRegion region = get_system_tray_region();
    printf("Capture region: %dx%d at (%d,%d)\n",
           region.width, region.height, region.x, region.y);

    if (!capture_screen_region(region.x, region.y,
                               region.width, region.height,
                               "current_layout.bmp"))
    {
        printf("Failed to capture screen\n");
        ocr_cleanup();
        return -1;
    }

    OCRResult result = ocr_from_file("current_layout.bmp", OCR_LANG_ENGLISH_RUSSIAN);
    int layout = -2;

    if (result.error_code == OCR_SUCCESS && result.text)
    {
        printf("OCR recognized: '%s'\n", result.text);

        // Simple text analysis for layout detection
        char lower_text[256];
        strncpy(lower_text, result.text, sizeof(lower_text) - 1);
        lower_text[sizeof(lower_text) - 1] = '\0';

        for (int i = 0; lower_text[i]; i++)
        {
            lower_text[i] = tolower(lower_text[i]);
        }

        if (strstr(lower_text, "rus") || strstr(lower_text, "ru ") ||
            strstr(result.text, "RU") || strstr(result.text, "RUS"))
        {
            layout = 1; // RUS
        }
        else if (strstr(lower_text, "eng") || strstr(lower_text, "en ") ||
                 strstr(result.text, "EN") || strstr(result.text, "ENG"))
        {
            layout = 0; // ENG
        }
        else
        {
            printf("Could not determine layout from OCR text\n");
        }
    }
    else
    {
        printf("OCR failed with error: %d\n", result.error_code);
    }

    ocr_result_free(&result);
    ocr_cleanup();

    return layout;
}

/* Main keyboard layout detection function */
int detect_keyboard_layout()
{
    printf("=== Keyboard Layout Detection ===\n\n");

    // First try Windows API
    printf("1. Windows API method:\n");
    int api_layout = detect_layout_windows_api();

    if (api_layout >= 0)
    {
        // Direct string output without using layout_to_string
        if (api_layout == 0)
        {
            printf("   Result: ENG (English)\n\n");
        }
        else if (api_layout == 1)
        {
            printf("   Result: RUS (Russian)\n\n");
        }
        else
        {
            printf("   Result: UNKNOWN\n\n");
        }
        return api_layout;
    }

    // Fallback to OCR
    printf("2. OCR method:\n");
    int ocr_layout = detect_layout_ocr();

    if (ocr_layout >= 0)
    {
        if (ocr_layout == 0)
        {
            printf("   Result: ENG (English)\n\n");
        }
        else if (ocr_layout == 1)
        {
            printf("   Result: RUS (Russian)\n\n");
        }
        else
        {
            printf("   Result: UNKNOWN\n\n");
        }
        return ocr_layout;
    }

    printf("Failed to detect keyboard layout\n");
    return -1;
}

/* Main function */
int main(int argc, char *argv[])
{
    printf("=========================================\n");
    printf("   Keyboard Layout Detector v1.0\n");
    printf("=========================================\n\n");

    int layout = detect_keyboard_layout();

    printf("=========================================\n");
    printf("FINAL LAYOUT: %s\n", layout >= 0 ? (layout == 0 ? "ENG" : "RUS") : "UNKNOWN");
    printf("=========================================\n");

    // Additional information
    printf("\nAdditional information:\n");
    printf("- Screen resolution: %dx%d\n", get_screen_width(), get_screen_height());

    ScreenRegion tray = get_system_tray_region();
    printf("- Indicator region: %dx%d at (%d,%d)\n",
           tray.width, tray.height, tray.x, tray.y);

    // Save debug screenshot
    if (capture_screen_region(tray.x, tray.y, tray.width, tray.height, "debug_indicator.bmp"))
    {
        printf("- Screenshot saved: debug_indicator.bmp\n");
    }

    printf("\nUsage:\n");
    printf("  No parameters - automatic detection\n");
    printf("  -o            - use OCR method only\n");
    printf("  -w            - use Windows API only\n");

    return 0;
}