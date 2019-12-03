#ifndef TOMATBOOT_UEFI_DEBUG_EXCEPTION_H
#define TOMATBOOT_UEFI_DEBUG_EXCEPTION_H
#include <Uefi.h>

struct idt_ptr_t {
    UINT16 size;
    /* Start address */
    UINT64 address;
} __attribute((packed));

struct idt_entry_t {
    UINT16 offset_low;
    UINT16 selector;
    UINT8 ist;
    UINT8 type_attr;
    UINT16 offset_mid;
    UINT32 offset_high;
    UINT32 zero;
} __attribute((packed));

struct regs_t {
    UINT64 Cr0;
    UINT64 Cr2;
    UINT64 Cr3;
    UINT64 Cr4;
    UINT64 Ldtr;
    UINT64 Tr;
    UINT64 Gdtr[2];
    UINT64 Idtr[2];
    UINT64 Gs;
    UINT64 Fs;
    UINT64 Es;
    UINT64 Ds;
    UINT64 R15;
    UINT64 R14;
    UINT64 R13;
    UINT64 R12;
    UINT64 R11;
    UINT64 R10;
    UINT64 R9;
    UINT64 R8;
    UINT64 Rbp;
    UINT64 Rdi;
    UINT64 Rsi;
    UINT64 Rdx;
    UINT64 Rcx;
    UINT64 Rbx;
    UINT64 Rax;
    UINT64 ExceptionData;
    UINT64 Rip;
    UINT64 Cs;
    UINT64 Rflags;
    UINT64 Rsp;
    UINT64 Ss;
};

void hook_idt();
void unhook_idt();
#endif

