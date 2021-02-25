#ifndef __TOMATBOOT_VMM_H__
#define __TOMATBOOT_VMM_H__

#include <Uefi.h>

typedef struct _PAGE_MAP {
    int Levels;
    void* TopLevel;
} PAGE_MAP;

/**
 * We are using 2mb pages for saving a bit of space
 */
#define PAGE_SIZE SIZE_2MB

/**
 * Create a new page table, this is mostly used in the loaders that give
 * a direct higher half.
 *
 * @param PageMap   [IN] The new page map
 * @param levels    [IN] The amount of levels (4 vs 5)
 */
EFI_STATUS NewPagemap(PAGE_MAP* PageMap, int levels);

/**
 * Map a page into the given page tables
 *
 * @param PageMap   [IN] The new page map
 * @param Vaddr     [IN] The virtual address to map
 * @param Paddr     [IN] The physical address to map
 * @param Flags     [IN] The flags to use for the mapping
 */
EFI_STATUS MapPage(PAGE_MAP* PageMap, EFI_VIRTUAL_ADDRESS Vaddr, EFI_PHYSICAL_ADDRESS Paddr);

#endif //__TOMATBOOT_VMM_H__
