#include <stdlib.h>
#include <uefi/uefi.h>

void* malloc(size_t s) {
    void* ptr;
    gBS->AllocatePool(EfiBootServicesData, s + sizeof(size_t), &ptr);
    *(size_t*)ptr = s;
    return (void*)((char*)ptr + sizeof(size_t));
}

void* calloc(size_t size, size_t elemSize) {
    void* new = malloc(size * elemSize);
    gBS->SetMem(new, size, 0);
    return new;
}

void* realloc(void* ptr, size_t new_size) {
    size_t old_size = *(size_t*)((char*)ptr - sizeof(size_t));
    if(old_size >= new_size) {
        return ptr;
    }
    char* new = (char*)malloc(new_size + sizeof(size_t));
    *(size_t*)new = new_size;
    gBS->CopyMem(new + sizeof(size_t), ptr, old_size);
    return new + sizeof(size_t);
}

void free(void* ptr) {
    gBS->FreePool((char*)ptr - sizeof(size_t));
}
