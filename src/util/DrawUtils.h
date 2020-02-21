#include <ProcessorBind.h>

#ifndef __UTIL_DRAWUTILS_H__
#define __UTIL_DRAWUTILS_H__

void WriteAt(int x, int y, const CHAR8* fmt, ...);
void DrawImage(int x, int y, CHAR8 image[], int width, int height);
void ClearScreen(CHAR8 color);
void FillBox(int _x, int _y, int width, int height, CHAR8 color);

#endif //__UTIL_DRAWUTILS_H__
