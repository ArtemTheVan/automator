#include <windows.h>
#include "automator.h"
#include "log.h"

/* Общее состояние библиотеки. */
int g_keystroke_delay_ms = 10;
int g_mouse_delay_ms = 50;

const char *automator_version(void)
{
    return AUTOMATOR_VERSION;
}

void set_keystroke_delay_ms(int ms)
{
    if (ms < 0) ms = 0;
    g_keystroke_delay_ms = ms;
}

void set_mouse_delay_ms(int ms)
{
    if (ms < 0) ms = 0;
    g_mouse_delay_ms = ms;
}

/*
 * Включает per-monitor DPI awareness через user32!SetProcessDpiAwarenessContext
 * (Windows 10 1703+). Если функция недоступна — пробуем shcore!SetProcessDpiAwareness
 * (Windows 8.1+), затем user32!SetProcessDPIAware (Windows Vista+).
 * Возвращает 1, если хоть какой-то режим включён.
 */
int automator_init(void)
{
    typedef BOOL (WINAPI *PFN_SetProcessDpiAwarenessContext)(HANDLE);
    typedef HRESULT (WINAPI *PFN_SetProcessDpiAwareness)(int);
    typedef BOOL (WINAPI *PFN_SetProcessDPIAware)(void);

    /* DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 = -4 */
    HMODULE user32 = GetModuleHandleA("user32.dll");
    if (user32) {
        PFN_SetProcessDpiAwarenessContext set_ctx =
            (PFN_SetProcessDpiAwarenessContext)GetProcAddress(user32, "SetProcessDpiAwarenessContext");
        if (set_ctx && set_ctx((HANDLE)-4)) {
            LOG_INFO("DPI awareness: PerMonitorV2");
            return 1;
        }
    }

    HMODULE shcore = LoadLibraryA("shcore.dll");
    if (shcore) {
        PFN_SetProcessDpiAwareness set_aw =
            (PFN_SetProcessDpiAwareness)GetProcAddress(shcore, "SetProcessDpiAwareness");
        if (set_aw && set_aw(2 /* PROCESS_PER_MONITOR_DPI_AWARE */) >= 0) {
            LOG_INFO("DPI awareness: PerMonitor (shcore)");
            return 1;
        }
        FreeLibrary(shcore);
    }

    if (user32) {
        PFN_SetProcessDPIAware legacy =
            (PFN_SetProcessDPIAware)GetProcAddress(user32, "SetProcessDPIAware");
        if (legacy && legacy()) {
            LOG_INFO("DPI awareness: System");
            return 1;
        }
    }

    LOG_WARN("DPI awareness API not available");
    return 0;
}
