#include "string.h"

#include <stdint.h>

void* memcpy(void* destptr, const void* srcptr, size_t num) {
    uint8_t* dest = destptr;
    const uint8_t* src = srcptr;
    while(num--) {
        *dest++ = *src++;
    }
    return destptr;
}

//void* memmove(void* destptr, const void* srcptr, size_t num) {
//    uint8_t* dest = destptr;
//    const uint8_t* src = srcptr;
//    uint8_t tmp[num];
//    memcpy(tmp, src, num);
//    memcpy(dest, tmp, num);
//    return dest;
//}

int memcmp(const void* ptr1, const void* ptr2, size_t num) {
    const uint8_t* buf1 = ptr1;
    const uint8_t* buf2 = ptr2;
    while(num) {
        if(*buf1 != *buf2) {
            return *buf1 - *buf2;
        }
        buf1++;
        buf2++;
        num--;
    }

    return 0;
}

void* memchr(const void* ptr, int value, size_t num) {
    const uint8_t* buf = ptr;
    while(num) {
        if(*buf == (uint8_t)value) {
            return (void*)buf;
        }
        buf++;
    }
    return NULL;
}

void* memset(void* ptr, int value, size_t num) {
    uint8_t val8 = (uint8_t)(value);
    uint8_t* buf = ptr;
    while(num--) {
        *buf++ = val8;
    }
    return ptr;
}

size_t strlen(const char* str) {
    size_t len = 0;
	while (str[len++] != 0);
	return len;
}

int strcmp(const char* str1, const char* str2) {
    while(*str1++ == *str2++ && *str1 != 0);
    return *(str1 - 1) - *(str2 - 1);
}

size_t wcslen(const wchar_t* str) {
    size_t len = 0;
	while (str[len++] != 0);
	return len;
}

size_t wcscmp(const wchar_t* str1, const wchar_t* str2) {
    size_t len = 0;
	wchar_t c = 0;
	while ((c = str1[len++]) != 0) {
		if (c != str2[len-1]) {
			return 0;
		}
	}

	return 1;
}
