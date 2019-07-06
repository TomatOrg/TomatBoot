#ifndef KRETLIM_UEFI_BOOT_STDARG_H
#define KRETLIM_UEFI_BOOT_STDARG_H

/**
 * type for iterating arguments
 *
 * @since C89
 */
typedef __builtin_va_list va_list;

/**
 * Start iterating arguments with a va_list
 *
 * @since C89
 */
#define va_start(v,l)	    __builtin_va_start(v,l)

/**
 * Retrieve an argument
 *
 * @since C89
 */
#define va_end(v)	        __builtin_va_end(v)

/**
 * Free a va_list
 *
 * @since C89
 */
#define va_arg(v,l)	        __builtin_va_arg(v,l)

/**
 * Copy contents of one va_list to another
 *
 * @since C99
 */
#define va_copy(d,s)	    __builtin_va_copy(d,s)

#endif //KRETLIM_UEFI_BOOT_STDARG_H
