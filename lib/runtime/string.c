#include <Uefi.h>
#include <Library/BaseMemoryLib.h>

void memset(void* dest, int value, UINTN len) {
    SetMem(dest, len, value);
}
