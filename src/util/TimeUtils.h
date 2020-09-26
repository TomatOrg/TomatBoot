#ifndef __UEFI_TIMEUTILS_H__
#define __UEFI_TIMEUTILS_H__

#include <ProcessorBind.h>

UINT64 GetUnixEpoch(UINT8 seconds, UINT8 minutes, UINT8  hours, UINT8 days, UINT8 months, UINT8 years);

#endif //__UEFI_TIMEUTILS_H__
