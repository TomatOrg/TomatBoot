#include <Library/BaseLib.h>
#include <Register/Intel/Cpuid.h>

#include "Cpu.h"

BOOLEAN Check5LevelPagingSupport() {
    CPUID_VIR_PHY_ADDRESS_SIZE_EAX VirPhyAddressSize = {0};
    CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_ECX ExtFeatureEcx = {0};
    UINT32 MaxExtendedFunctionId = 0;

    // query the virtual address size
    AsmCpuid (CPUID_EXTENDED_FUNCTION, &MaxExtendedFunctionId, NULL, NULL, NULL);
    if (MaxExtendedFunctionId >= CPUID_VIR_PHY_ADDRESS_SIZE) {
        AsmCpuid (CPUID_VIR_PHY_ADDRESS_SIZE, &VirPhyAddressSize.Uint32, NULL, NULL, NULL);
    } else {
        VirPhyAddressSize.Bits.PhysicalAddressBits = 36;
    }

    // query the five level paging support
    AsmCpuidEx (
            CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS,
            CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_SUB_LEAF_INFO,
            NULL, NULL, &ExtFeatureEcx.Uint32, NULL
    );

    // check we have five level paging and a big enough virtual address space
    if (ExtFeatureEcx.Bits.FiveLevelPage == 1 && VirPhyAddressSize.Bits.PhysicalAddressBits > 4 * 9 + 12) {
        return TRUE;
    } else {
        return FALSE;
    }
}
