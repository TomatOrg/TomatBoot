#ifndef TOMATBOOT_UEFI_FILE_UTILS_H
#define TOMATBOOT_UEFI_FILE_UTILS_H

void load_file(const CHAR8* path, int memory_type, UINT64* out_base, UINT64* out_len);

#endif //TOMATBOOT_UEFI_FILE_UTILS_H
