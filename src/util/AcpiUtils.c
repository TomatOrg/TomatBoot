#include <Uefi.h>
#include <Guid/Acpi.h>
#include "AcpiUtils.h"
#include "Except.h"

void* GetAcpiTable(UINT32 Signature) {
    void* AcpiTable = NULL;

    BOOLEAN IsDsdt = (Signature == EFI_ACPI_1_0_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE);
    UINT64* Entries64 = NULL;
    UINT32* Entries32 = NULL;
    UINTN Count = 0;

    if (IsDsdt) {
        Signature = EFI_ACPI_1_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE;
    }

    // get the acpi rsdp
    if (!EFI_ERROR(EfiGetSystemConfigurationTable(&gEfiAcpi20TableGuid, &AcpiTable))) {
        EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER* Rsdp = AcpiTable;
        if (Rsdp->XsdtAddress != 0) {
            EFI_ACPI_DESCRIPTION_HEADER* Xsdt = (EFI_ACPI_DESCRIPTION_HEADER*)(UINTN)Rsdp->XsdtAddress;
            Entries64 = (UINT64*)(Xsdt + 1);
            Count = (Xsdt->Length - sizeof(EFI_ACPI_DESCRIPTION_HEADER)) / sizeof(UINT64);
        } else if (Rsdp->RsdtAddress != 0) {
            EFI_ACPI_DESCRIPTION_HEADER* Rsdt = (EFI_ACPI_DESCRIPTION_HEADER*)(UINTN)Rsdp->RsdtAddress;
            Entries64 = (UINT64*)(Rsdt + 1);
            Count = (Rsdt->Length - sizeof(EFI_ACPI_DESCRIPTION_HEADER)) / sizeof(UINT32);
        } else {
            return NULL;
        }
    } else if (!EFI_ERROR(EfiGetSystemConfigurationTable(&gEfiAcpi10TableGuid, &AcpiTable))) {
        EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_POINTER* Rsdp = AcpiTable;
        if (Rsdp->RsdtAddress != 0) {
            EFI_ACPI_DESCRIPTION_HEADER* Rsdt = (EFI_ACPI_DESCRIPTION_HEADER*)(UINTN)Rsdp->RsdtAddress;
            Entries64 = (UINT64*)(Rsdt + 1);
            Count = (Rsdt->Length - sizeof(EFI_ACPI_DESCRIPTION_HEADER)) / sizeof(UINT32);
        } else {
            return NULL;
        }
    } else {
        // no acpi support :(
        return NULL;
    }

    // search for the table
    EFI_ACPI_DESCRIPTION_HEADER* Table = NULL;
    for (int i = 0; i < Count; i++) {
        Table = NULL;
        if (Entries64) {
            Table = (EFI_ACPI_DESCRIPTION_HEADER*)Entries64[i];
        } else {
            Table = (EFI_ACPI_DESCRIPTION_HEADER*)(UINTN)Entries32[i];
        }

        if (Table->Signature == Signature) {
            break;
        }
    }

    if (Table == NULL) {
        return NULL;
    }

    // take the dsdt if this is what we wanted
    if (IsDsdt) {
        if (Table->Revision >= EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE_REVISION) {
            EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE* Facp = (EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE*)Table;
            if (Facp->XDsdt != 0) {
                return (void*)(UINTN)Facp->XDsdt;
            } else if (Facp->Dsdt != 0) {
                return (void*)(UINTN)Facp->Dsdt;
            } else {
                return NULL;
            }
        } else {
            EFI_ACPI_1_0_FIXED_ACPI_DESCRIPTION_TABLE* Facp = (EFI_ACPI_1_0_FIXED_ACPI_DESCRIPTION_TABLE*)Table;
            if (Facp->Dsdt != 0) {
                return (void*)(UINTN)Facp->Dsdt;
            } else {
                return NULL;
            }
        }
    } else {
        return Table;
    }
}
