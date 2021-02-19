#include <ProcessorBind.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include "StringUtils.h"
#include "Except.h"

CHAR16* StrDup(CHAR16* Original) {
    ASSERT(Original != NULL);
    return AllocateCopyPool(StrSize(Original), Original);
}

CHAR8* ConvertToAscii(CHAR16* Original) {
    ASSERT(Original != NULL);
    UINTN CmdlineLen = StrLen(Original) + 1;
    CHAR8* Cmdline = AllocateZeroPool(CmdlineLen);
    if (Cmdline == NULL) {
        return NULL;
    }
    UnicodeStrToAsciiStrS(Original, Cmdline, CmdlineLen);
    return Cmdline;
}
