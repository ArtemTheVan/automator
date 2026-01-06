#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "window.h"
#include "screen.h" /* Добавляем этот include */

/* Вспомогательная структура для передачи данных в callback-функции */
typedef struct
{
    WindowList *list;
    const char *search_class;
    const char *search_title;
    const char *search_text;
    BOOL class_match;
    BOOL title_match;
    BOOL text_match;
} EnumWindowsData;

/* Callback-функция для EnumWindows */
static BOOL CALLBACK enum_windows_callback(HWND hwnd, LPARAM lParam)
{
    EnumWindowsData *data = (EnumWindowsData *)lParam;

    /* Проверяем видимость окна (опционально, можно убрать для отладки) */
    if (!IsWindowVisible(hwnd))
        return TRUE;

    /* Получаем заголовок окна */
    char title[256];
    GetWindowTextA(hwnd, title, sizeof(title));
    title[sizeof(title) - 1] = '\0'; /* Гарантируем null-terminated */

    /* Получаем класс окна */
    char class_name[256];
    GetClassNameA(hwnd, class_name, sizeof(class_name));
    class_name[sizeof(class_name) - 1] = '\0'; /* Гарантируем null-terminated */

    /* Проверяем соответствие критериям поиска */
    BOOL matches = TRUE;

    if (data->class_match && data->search_class && data->search_class[0])
    {
        if (strstr(class_name, data->search_class) == NULL)
            matches = FALSE;
    }

    if (data->title_match && data->search_title && data->search_title[0])
    {
        if (strstr(title, data->search_title) == NULL)
            matches = FALSE;
    }

    /* Если окно соответствует критериям, добавляем его в список */
    if (matches)
    {
        /* Увеличиваем емкость списка при необходимости */
        if (data->list->count >= data->list->capacity)
        {
            int new_capacity = data->list->capacity * 2;
            WindowInfo *new_windows = realloc(data->list->windows,
                                              sizeof(WindowInfo) * new_capacity);
            if (!new_windows)
                return FALSE; /* Ошибка выделения памяти */

            data->list->windows = new_windows;
            data->list->capacity = new_capacity;
        }

        /* Заполняем информацию об окне */
        WindowInfo *info = &data->list->windows[data->list->count];
        info->handle = hwnd;
        strncpy(info->title, title, sizeof(info->title) - 1);
        info->title[sizeof(info->title) - 1] = '\0';
        strncpy(info->class_name, class_name, sizeof(info->class_name) - 1);
        info->class_name[sizeof(info->class_name) - 1] = '\0';
        GetWindowRect(hwnd, &info->rect);
        info->is_visible = IsWindowVisible(hwnd);

        DWORD pid;
        GetWindowThreadProcessId(hwnd, &pid);
        info->pid = pid;

        data->list->count++;
    }

    return TRUE; /* Продолжаем перечисление */
}

/* Создает новый пустой список окон */
static WindowList *create_window_list(void)
{
    WindowList *list = malloc(sizeof(WindowList));
    if (!list)
        return NULL;

    list->capacity = 10;
    list->count = 0;
    list->windows = malloc(sizeof(WindowInfo) * list->capacity);

    if (!list->windows)
    {
        free(list);
        return NULL;
    }

    return list;
}

/* Основные функции API */

WindowList *find_windows_by_class(const char *class_name)
{
    WindowList *list = create_window_list();
    if (!list)
        return NULL;

    EnumWindowsData data = {
        .list = list,
        .search_class = class_name,
        .search_title = NULL,
        .search_text = NULL,
        .class_match = TRUE,
        .title_match = FALSE,
        .text_match = FALSE};

    EnumWindows(enum_windows_callback, (LPARAM)&data);

    return list;
}

WindowList *find_windows_by_title(const char *title_pattern)
{
    WindowList *list = create_window_list();
    if (!list)
        return NULL;

    EnumWindowsData data = {
        .list = list,
        .search_class = NULL,
        .search_title = title_pattern,
        .search_text = NULL,
        .class_match = FALSE,
        .title_match = TRUE,
        .text_match = FALSE};

    EnumWindows(enum_windows_callback, (LPARAM)&data);

    return list;
}

WindowList *find_all_visible_windows(void)
{
    WindowList *list = create_window_list();
    if (!list)
        return NULL;

    EnumWindowsData data = {
        .list = list,
        .search_class = NULL,
        .search_title = NULL,
        .search_text = NULL,
        .class_match = FALSE,
        .title_match = FALSE,
        .text_match = FALSE};

    EnumWindows(enum_windows_callback, (LPARAM)&data);

    return list;
}

void free_window_list(WindowList *list)
{
    if (list)
    {
        if (list->windows)
            free(list->windows);
        free(list);
    }
}

/* Функции для получения информации об окнах */

BOOL get_window_rect_ex(HWND hwnd, RECT *rect)
{
    if (!hwnd || !rect)
        return FALSE;

    return GetWindowRect(hwnd, rect);
}

BOOL get_window_info_ex(HWND hwnd, WindowInfo *info)
{
    if (!hwnd || !info)
        return FALSE;

    info->handle = hwnd;
    GetWindowTextA(hwnd, info->title, sizeof(info->title));
    info->title[sizeof(info->title) - 1] = '\0';
    GetClassNameA(hwnd, info->class_name, sizeof(info->class_name));
    info->class_name[sizeof(info->class_name) - 1] = '\0';
    GetWindowRect(hwnd, &info->rect);
    info->is_visible = IsWindowVisible(hwnd);

    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);
    info->pid = pid;

    return TRUE;
}

BOOL is_window_visible_ex(HWND hwnd)
{
    if (!hwnd)
        return FALSE;

    return IsWindowVisible(hwnd);
}

char *get_window_text_ex(HWND hwnd, char *buffer, int buffer_size)
{
    if (!hwnd || !buffer || buffer_size <= 0)
        return NULL;

    GetWindowTextA(hwnd, buffer, buffer_size);
    buffer[buffer_size - 1] = '\0';
    return buffer;
}

char *get_window_class_ex(HWND hwnd, char *buffer, int buffer_size)
{
    if (!hwnd || !buffer || buffer_size <= 0)
        return NULL;

    GetClassNameA(hwnd, buffer, buffer_size);
    buffer[buffer_size - 1] = '\0';
    return buffer;
}

/* Функции поиска системного трея */

HWND find_system_tray_window(void)
{
    HWND hwnd = NULL;

    /* 1. Ищем стандартный трей Windows */
    hwnd = FindWindow("Shell_TrayWnd", NULL);
    if (hwnd)
    {
        /* Ищем дочернее окно с иконками трея */
        HWND tray_notify = FindWindowEx(hwnd, NULL, "TrayNotifyWnd", NULL);
        if (tray_notify)
        {
            /* Ищем ToolbarWindow32 внутри TrayNotifyWnd */
            HWND toolbar = FindWindowEx(tray_notify, NULL, "ToolbarWindow32", NULL);
            if (toolbar)
                return toolbar;

            return tray_notify;
        }
    }

    /* 2. Ищем трей для дополнительных мониторов */
    hwnd = FindWindow("Shell_SecondaryTrayWnd", NULL);
    if (hwnd)
    {
        HWND toolbar = FindWindowEx(hwnd, NULL, "ToolbarWindow32", NULL);
        if (toolbar)
            return toolbar;

        return hwnd;
    }

    /* 3. Ищем окно переполнения иконок трея */
    hwnd = FindWindow("NotifyIconOverflowWindow", NULL);
    if (hwnd)
        return hwnd;

    /* 4. Ищем трей по известным классам */
    const char *tray_classes[] = {
        "Windows.UI.Core.CoreWindow", /* Windows 10/11 */
        "Shell_TrayWnd",
        "Shell_SecondaryTrayWnd",
        NULL};

    for (int i = 0; tray_classes[i]; i++)
    {
        hwnd = FindWindow(tray_classes[i], NULL);
        if (hwnd)
        {
            /* Проверяем, содержит ли окно иконки */
            HWND child = FindWindowEx(hwnd, NULL, "ToolbarWindow32", NULL);
            if (child)
                return child;

            return hwnd;
        }
    }

    return NULL;
}

HWND find_taskbar_window(void)
{
    return FindWindow("Shell_TrayWnd", NULL);
}

ScreenRegion get_system_tray_region_ex(void)
{
    ScreenRegion region = {0};

    /* 1. Пытаемся найти окно системного трея */
    HWND tray_window = find_system_tray_window();

    if (tray_window)
    {
        RECT rect;
        if (GetWindowRect(tray_window, &rect))
        {
            region.x = rect.left;
            region.y = rect.top;
            region.width = rect.right - rect.left;
            region.height = rect.bottom - rect.top;

            printf("Found system tray window: %dx%d at (%d,%d)\n",
                   region.width, region.height, region.x, region.y);

            /* Увеличиваем область для лучшего захвата (если нужно) */
            int padding = 5;
            region.x -= padding;
            region.y -= padding;
            region.width += padding * 2;
            region.height += padding * 2;

            return region;
        }
    }

    /* 2. Если трей не нашли, ищем панель задач */
    HWND taskbar = find_taskbar_window();
    if (taskbar)
    {
        RECT rect;
        if (GetWindowRect(taskbar, &rect))
        {
            /* Определяем положение панели задач */
            int screen_width = GetSystemMetrics(SM_CXSCREEN);
            int screen_height = GetSystemMetrics(SM_CYSCREEN);

            /* Панель задач обычно внизу, но может быть сверху, слева или справа */
            if (rect.bottom == screen_height && rect.top > 0)
            {
                /* Панель внизу - трей справа */
                region.width = 200;
                region.height = rect.bottom - rect.top;
                region.x = rect.right - region.width;
                region.y = rect.top;
            }
            else if (rect.top == 0 && rect.bottom < screen_height)
            {
                /* Панель сверху - трей справа */
                region.width = 200;
                region.height = rect.bottom - rect.top;
                region.x = rect.right - region.width;
                region.y = rect.top;
            }
            else if (rect.left == 0 && rect.right < screen_width)
            {
                /* Панель слева - трей внизу */
                region.width = rect.right - rect.left;
                region.height = 40;
                region.x = rect.left;
                region.y = rect.bottom - region.height;
            }
            else if (rect.right == screen_width && rect.left > 0)
            {
                /* Панель справа - трей внизу */
                region.width = rect.right - rect.left;
                region.height = 40;
                region.x = rect.left;
                region.y = rect.bottom - region.height;
            }

            printf("Using taskbar for system tray: %dx%d at (%d,%d)\n",
                   region.width, region.height, region.x, region.y);

            return region;
        }
    }

    /* 3. Fallback: используем эвристику (правый нижний угол) */
    int screen_width = GetSystemMetrics(SM_CXSCREEN);
    int screen_height = GetSystemMetrics(SM_CYSCREEN);

    region.width = 200;
    region.height = 50;
    region.x = screen_width - region.width - 10;
    region.y = screen_height - region.height - 10;

    printf("Using fallback for system tray: %dx%d at (%d,%d)\n",
           region.width, region.height, region.x, region.y);

    return region;
}

ScreenRegion get_taskbar_region(void)
{
    ScreenRegion region = {0};
    HWND taskbar = find_taskbar_window();

    if (taskbar)
    {
        RECT rect;
        if (GetWindowRect(taskbar, &rect))
        {
            region.x = rect.left;
            region.y = rect.top;
            region.width = rect.right - rect.left;
            region.height = rect.bottom - rect.top;
        }
    }

    return region;
}

/* Вспомогательные функции */

void print_window_info(const WindowInfo *info)
{
    if (!info)
        return;

    printf("Window: HWND=0x%p\n", info->handle);
    printf("  Title: %s\n", info->title);
    printf("  Class: %s\n", info->class_name);
    printf("  Position: (%ld,%ld)-(%ld,%ld) [%ldx%ld]\n",
           info->rect.left, info->rect.top,
           info->rect.right, info->rect.bottom,
           info->rect.right - info->rect.left,
           info->rect.bottom - info->rect.top);
    printf("  PID: %d\n", info->pid);
    printf("  Visible: %s\n", info->is_visible ? "Yes" : "No");
}

void print_window_list(const WindowList *list)
{
    if (!list)
        return;

    printf("Found %d windows:\n", list->count);
    for (int i = 0; i < list->count; i++)
    {
        printf("[%d] ", i);
        print_window_info(&list->windows[i]);
        printf("\n");
    }
}

HWND find_window_containing_text(const char *text)
{
    HWND result = NULL;

    WindowList *list = find_all_visible_windows();
    if (!list)
        return NULL;

    for (int i = 0; i < list->count; i++)
    {
        if (strstr(list->windows[i].title, text) != NULL)
        {
            result = list->windows[i].handle;
            break;
        }
    }

    free_window_list(list);
    return result;
}

HWND find_virtual_machine_window(void)
{
    /* Список известных окон виртуальных машин */
    const char *vm_titles[] = {
        "VirtualBox",
        "VMware",
        "Virtual Machine",
        "Hyper-V",
        "QEMU",
        "Parallels",
        "Virt-Manager",
        NULL};

    WindowList *list = find_all_visible_windows();
    if (!list)
        return NULL;

    HWND result = NULL;

    for (int i = 0; i < list->count; i++)
    {
        const char *title = list->windows[i].title;
        for (int j = 0; vm_titles[j]; j++)
        {
            if (strstr(title, vm_titles[j]) != NULL)
            {
                result = list->windows[i].handle;
                break;
            }
        }

        if (result)
            break;
    }

    free_window_list(list);
    return result;
}