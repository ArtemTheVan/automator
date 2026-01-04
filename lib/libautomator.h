#ifndef LIBAUTOMATOR_H_
#define LIBAUTOMATOR_H_

#include "keyboard.h"
#include "mouse.h"
#include "screen.h"
#include "ocr.h"

// #ifdef _WIN32
//     #include <windows.h>
//     #define PLATFORM_WINDOWS 1
// #elif __linux__
//     #include <X11/Xlib.h>
//     #include <X11/extensions/XTest.h>
//     #define PLATFORM_LINUX 1
// #endif

// #if PLATFORM_WINDOWS
//     // Windows-специфичные определения
// #elif PLATFORM_LINUX
//     // Linux-специфичные определения
// #endif

#ifdef BUILD_DLL
    #define AUTOMATOR_API __declspec(dllexport)
#else
    #define AUTOMATOR_API __declspec(dllimport)
#endif

#endif
