#include "TimeUtils.h"

// Julian date calculation from https://en.wikipedia.org/wiki/Julian_day
static UINT64 GetJdn(UINT8 days, UINT8 months, UINT16 years) {
    return (1461 * (years + 4800 + (months - 14)/12))/4 + (367 *
           (months - 2 - 12 * ((months - 14)/12)))/12 -
           (3 * ((years + 4900 + (months - 14)/12)/100))/4
           + days - 32075;
}

UINT64 GetUnixEpoch(UINT8 seconds, UINT8 minutes, UINT8  hours, UINT8 days, UINT8 months, UINT8 years) {
    UINT64 jdn_current = GetJdn(days, months, years);
    UINT64 jdn_1970    = GetJdn(1, 1, 1970);

    UINT64 jdn_diff = jdn_current - jdn_1970;

    return (jdn_diff * (60 * 60 * 24)) + hours * 3600 + minutes * 60 + seconds;
}

