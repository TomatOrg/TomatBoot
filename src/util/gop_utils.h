#ifndef TOMATBOOT_UEFI_GOP_UTILS_H
#define TOMATBOOT_UEFI_GOP_UTILS_H

#include <Uefi.h>

INT32 get_first_mode();
INT32 get_next_mode(INT32 current);
INT32 get_prev_mode(INT32 current);

#endif //TOMATBOOT_UEFI_GOP_UTILS_H
