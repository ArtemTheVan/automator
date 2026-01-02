#ifndef SCREEN_H_
#define SCREEN_H_

#include <windows.h>
#include <stdio.h>
#include <string.h>

int capture_screen_region(int x, int y, int width, int height, const char *filename);

#endif
