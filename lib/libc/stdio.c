#include "stdio.h"

#include <uefi/uefi.h>
#include <string.h>

static void print(const wchar_t* const string) {
	wchar_t* str = (wchar_t*)string;

	size_t len = wcslen(string);

	size_t last = 0;

	EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* out = gST->ConOut;

	size_t cols = 0;
	size_t rows = 0;

	out->QueryMode(out, out->Mode->Mode, &cols, &rows);

	for (size_t i = 0; i < len; i++) {
		if (str[i] == L'\n') {
			str[i] = 0;
			gST->ConOut->OutputString(gST->ConOut, str+last);
			
			size_t spaces = cols - out->Mode->CursorColumn;

			while (spaces--) {
				out->OutputString(out, L" ");
			}

			str[i] = L'\n';
			last = i+1;
		}
	}

	if (last != len) {
		gST->ConOut->OutputString(gST->ConOut, str+last);
	}
}

static void println(const wchar_t* const string) {
	print(string);
	print(L"\n");
}


#define FMT_PUT(dst, len, c) {\
				if(!(len)) goto end; \
				*(dst)++ = (c); \
				len--; \
			}

static const wchar_t *digits_upper = L"0123456789ABCDEF";
static const wchar_t *digits_lower = L"0123456789abcdef";

static wchar_t* num_fmt(wchar_t *buf, size_t buf_len, uint64_t i, int base, int padding, char pad_with, int handle_signed, int upper, int len) {
	int neg = (signed)i < 0 && handle_signed;

	if (neg)
		i = (unsigned)(-((signed)i));

	wchar_t *ptr = buf + buf_len - 1;
	*ptr = '\0';

	const wchar_t *digits = upper ? digits_upper : digits_lower;

	do {
		*--ptr = digits[i % base];
		if (padding)
			padding--;
		if (len > 0)
			len--;
		buf_len--;
	} while ((i /= base) != 0 && (len == -1 || len) && buf_len);

	while (padding && buf_len) {
		*--ptr = pad_with;
		padding--;
		buf_len--;
	}

	if (neg && buf_len)
		*--ptr = '-';

	return ptr;
}

static int isdigit(wchar_t c) {
	return c >= L'0' && c <= L'9';
}

static wchar_t printBuffer[2048];

void printf(const wchar_t* const format, ...) {
	va_list args;
	va_start(args, format);
	vsnprintf(printBuffer, 2048, format, args);
	va_end(args);

	print(printBuffer);
}

void vprintf(const wchar_t* format, va_list list) {
	vsnprintf(printBuffer, 2048, format, list);

	print(printBuffer);
}

size_t snprintf(wchar_t* buffer, size_t bufferSize, const wchar_t* format, ...) {

	va_list list;
	va_start(list, format);

	return vsnprintf(buffer, bufferSize, format, list);
}

#define NUM_BUF_LEN 48
size_t vsnprintf(wchar_t* buf, size_t len, const wchar_t* fmt, va_list arg) {
	uint64_t i;
	wchar_t *s;
	char* a;
	wchar_t num_buf[NUM_BUF_LEN];
	
	while(*fmt && len) {
		if (*fmt != '%') {
			*buf++ = *fmt;
			fmt++;
			continue;
		}

		fmt++;
		int padding = 0;
		wchar_t pad_with = ' ';
		int wide = 0, upper = 0;

		if (*fmt == '0') {
			pad_with = '0';
			fmt++;
		}

		while (isdigit(*fmt)) {
			padding *= 10;			// noop on first iter
			padding += *fmt++ - '0';
		}

		while (*fmt == 'l') {
			wide = 1;
			fmt++;
		}

		upper = *fmt == 'X' || *fmt == 'P';

		switch (*fmt) {
			case 'c': {
				i = va_arg(arg, int);
				FMT_PUT(buf, len, i)
				break;
			}

			case 'd': {
				if (wide)
					i = va_arg(arg, long int);
				else
					i = va_arg(arg, int);

				wchar_t *c = num_fmt(num_buf, NUM_BUF_LEN, i, 10, padding, pad_with, 1, 0, -1);
				while (*c) {
					FMT_PUT(buf, len, *c);
					c++;
				}
				break;
			}

			case 'u': {
				if (wide)
					i = va_arg(arg, long int);
				else
					i = va_arg(arg, int);

				wchar_t *c = num_fmt(num_buf, NUM_BUF_LEN, i, 10, padding, pad_with, 0, 0, -1);
				while (*c) {
					FMT_PUT(buf, len, *c);
					c++;
				}
				break;
			}

			case 'o': {
				if (wide)
					i = va_arg(arg, long int);
				else
					i = va_arg(arg, int);

				wchar_t *c = num_fmt(num_buf, NUM_BUF_LEN, i, 8, padding, pad_with, 0, 0, -1);
				while (*c) {
					FMT_PUT(buf, len, *c);
					c++;
				}
				break;
			}

			case 'X':
			case 'x': {
				if (wide)
					i = va_arg(arg, long int);
				else
					i = va_arg(arg, int);

				wchar_t *c = num_fmt(num_buf, NUM_BUF_LEN, i, 16, padding, pad_with, 0, upper, wide ? 16 : 8);
				while (*c) {
					FMT_PUT(buf, len, *c);
					c++;
				}
				break;
			}

			case 'P':
			case 'p': {
				i = (uint64_t)(va_arg(arg, void *));

				wchar_t *c = num_fmt(num_buf, NUM_BUF_LEN, i, 16, padding, pad_with, 0, upper, 16);
				while (*c) {
					FMT_PUT(buf, len, *c);
					c++;
				}
				break;
			}

			case 's': {
				s = va_arg(arg, wchar_t *);
				while (*s) {
					FMT_PUT(buf, len, *s);
					s++;
				}
				break;
			}

			case 'a': {
				a = va_arg(arg, char *);
				while (*a) {
					FMT_PUT(buf, len, (wchar_t)*a);
					a++;
				}
				break;
			}

			case '%': {
				FMT_PUT(buf, len, '%');
				break;
			}
		}

		fmt++;
	}

end:
	if (!len) {	
		*buf++ = '.';	// requires extra reserved space
		*buf++ = '.';
		*buf++ = '.';
	}

	*buf++ = '\0';
	return 0;
}

size_t fprintf(FILE* const f, const wchar_t* const format, ...) {
	EFI_FILE_PROTOCOL* const file = f;

	va_list list;	
	va_start(list, format);
	size_t written = vsnprintf(printBuffer, 2048, format, list) - 2;
	va_end(list);

	EFI_STATUS status = WriteFile(file, &written, printBuffer);

	if (status != EFI_SUCCESS) {
		written = 0;
		println(L"Failed to write to file\n");
	}

	return written;
}

