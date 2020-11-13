#ifndef __UTIL_GFXUTILS_H__
#define __UTIL_GFXUTILS_H__

#include <ProcessorBind.h>

INT32 GetFirstGfxMode();
INT32 GetNextGfxMode(INT32 Current);
INT32 GetPrevGfxMode(INT32 Current);

INT32 GetBestGfxMode(INT32 Width, INT32 Height);

#endif //__UTIL_GFXUTILS_H__
