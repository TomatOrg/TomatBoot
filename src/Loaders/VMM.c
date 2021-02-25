#include <Library/MemoryAllocationLib.h>

#include <Util/Except.h>
#include <Library/BaseMemoryLib.h>

#include "VMM.h"


#define PAGE_PRESENT    BIT0
#define PAGE_WRITE      BIT1
#define PAGE_HUGE       BIT7

/**
 * A helper function to return the next layer
 *
 * @param CurrentLevel
 * @param Entry
 */
static UINT64* GetNextLayer(UINT64* CurrentLevel, UINTN Entry) {
    if (CurrentLevel[Entry] & PAGE_PRESENT) {
        // page is present, return the address
        return (UINT64*)(CurrentLevel[Entry] & (~0xfffull));
    } else {
        // allocate a new page table level
        void* NewLevel = AllocatePages(1);
        if (NewLevel == NULL) {
            return NULL;
        }
        ZeroMem(NewLevel, EFI_PAGE_SIZE);

        // set the layer with rwx permissions
        CurrentLevel[Entry] = (UINT64)NewLevel | PAGE_PRESENT | PAGE_WRITE;
        return NewLevel;
    }
}

EFI_STATUS NewPagemap(PAGE_MAP* PageMap, int levels) {
    EFI_STATUS Status = EFI_SUCCESS;

    CHECK(PageMap != NULL);
    CHECK(levels == 4 || levels == 5);

    PageMap->Levels = levels;
    PageMap->TopLevel = AllocatePages(1);
    ZeroMem(PageMap->TopLevel, EFI_PAGE_SIZE);
    CHECK_STATUS(PageMap->TopLevel != NULL, EFI_OUT_OF_RESOURCES);

cleanup:
    return Status;
}

EFI_STATUS MapPage(PAGE_MAP* PageMap, EFI_VIRTUAL_ADDRESS Vaddr, EFI_PHYSICAL_ADDRESS Paddr) {
    EFI_STATUS Status = EFI_SUCCESS;

    CHECK(PageMap != NULL);

    // split to the different entries
    UINTN Pml5Entry = (Vaddr >> 48) & 0x1ff;
    UINTN Pml4Entry = (Vaddr >> 39) & 0x1ff;
    UINTN Pml3Entry = (Vaddr >> 30) & 0x1ff;
    UINTN Pml2Entry = (Vaddr >> 21) & 0x1ff;

    UINT64* CurrentLevel = PageMap->TopLevel;

    // handle 5 level paging
    if (PageMap->Levels == 5) {
        CurrentLevel = GetNextLayer(CurrentLevel, Pml5Entry);
        CHECK_STATUS(CurrentLevel != NULL, EFI_OUT_OF_RESOURCES);
    }

    // get the rest of the levels
    CurrentLevel = GetNextLayer(CurrentLevel, Pml4Entry);
    CHECK_STATUS(CurrentLevel != NULL, EFI_OUT_OF_RESOURCES);

    CurrentLevel = GetNextLayer(CurrentLevel, Pml3Entry);
    CHECK_STATUS(CurrentLevel != NULL, EFI_OUT_OF_RESOURCES);

    // set the page
    CurrentLevel[Pml2Entry] = Paddr | PAGE_PRESENT | PAGE_WRITE | PAGE_HUGE;

cleanup:
    return Status;
}
