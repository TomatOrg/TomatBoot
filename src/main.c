#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Protocol/Cpu.h>

#include <menus/menus.h>
#include <elf64/elf64.h>
#include "config.h"

EFI_HANDLE gImageHandle;
EFI_SYSTEM_TABLE* gST;
EFI_RUNTIME_SERVICES* gRT;
EFI_BOOT_SERVICES* gBS;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Exception handlers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int EXCEPTIONS[] = {
        EXCEPT_IA32_DIVIDE_ERROR,
        EXCEPT_IA32_DEBUG,
        EXCEPT_IA32_NMI,
        EXCEPT_IA32_BREAKPOINT,
        EXCEPT_IA32_OVERFLOW,
        EXCEPT_IA32_BOUND,
        EXCEPT_IA32_INVALID_OPCODE,
        EXCEPT_IA32_DOUBLE_FAULT,
        EXCEPT_IA32_INVALID_TSS,
        EXCEPT_IA32_SEG_NOT_PRESENT,
        EXCEPT_IA32_STACK_FAULT,
        EXCEPT_IA32_GP_FAULT,
        EXCEPT_IA32_PAGE_FAULT,
        EXCEPT_IA32_FP_ERROR,
        EXCEPT_IA32_ALIGNMENT_CHECK,
        EXCEPT_IA32_MACHINE_CHECK,
        EXCEPT_IA32_SIMD,
};

static const char* EXCEPTION_NAMES[] = {
        [EXCEPT_IA32_DIVIDE_ERROR] = "Divide by zero",
        [EXCEPT_IA32_DEBUG] = "Debug",
        [EXCEPT_IA32_NMI] = "Non-Maskable Interrupt (NMI)",
        [EXCEPT_IA32_BREAKPOINT] = "Breakpoint",
        [EXCEPT_IA32_OVERFLOW] = "Overflow",
        [EXCEPT_IA32_BOUND] = "Bound Range Exceeded",
        [EXCEPT_IA32_INVALID_OPCODE] = "Invalid opcode",
        [EXCEPT_IA32_DOUBLE_FAULT] = "Double Fault",
        [EXCEPT_IA32_INVALID_TSS] = "Invalid TSS",
        [EXCEPT_IA32_SEG_NOT_PRESENT] = "Segment not presented",
        [EXCEPT_IA32_STACK_FAULT] = "Stack segment Fault",
        [EXCEPT_IA32_GP_FAULT] = "General Protection Fault",
        [EXCEPT_IA32_PAGE_FAULT] = "Page Fault",
        [EXCEPT_IA32_FP_ERROR] = "x87 Floating Point Exception",
        [EXCEPT_IA32_ALIGNMENT_CHECK] = "Alignment Check",
        [EXCEPT_IA32_MACHINE_CHECK] = "Machine Check",
        [EXCEPT_IA32_SIMD] = "SIMD Floating Point Exception",
};

static char* TABLE_NAME[] = {
        "GDT",
        "IDT",
        "LDT",
        "IDT"
};

static void interrupt_handler(const EFI_EXCEPTION_TYPE InterruptType, const EFI_SYSTEM_CONTEXT SystemContext) {
    EFI_SYSTEM_CONTEXT_X64* ctx = SystemContext.SystemContextX64;

    // print the exception itself
    if(InterruptType < ARRAY_SIZE(EXCEPTION_NAMES) && EXCEPTION_NAMES[InterruptType] != 0) {
        DebugPrint(0, "Exception: %a\n", EXCEPTION_NAMES[InterruptType]);
    }else {
        DebugPrint(0, "Exception: %d\n", InterruptType);
    }

    // print the exception data if needed
    switch(InterruptType) {
        // has a selector
        case EXCEPT_IA32_INVALID_TSS:
        case EXCEPT_IA32_SEG_NOT_PRESENT:
        case EXCEPT_IA32_STACK_FAULT:
        case EXCEPT_IA32_GP_FAULT: {
            if(ctx->ExceptionData != 0) {
                const char* processor = ctx->ExceptionData & BIT0 ? "External" : "Internal";
                const char* table = TABLE_NAME[(ctx->ExceptionData >> 1u) & 0b11];
                int index = ctx->ExceptionData & 0xFFF8;
                DebugPrint(0, "Selector(processor=%a, table=%a, index=%x)\n", processor, table, index);
            }
        } break;

        // has a reason
        case EXCEPT_IA32_PAGE_FAULT: {
            DebugPrint(0, "Reason: %a\n", ctx->ExceptionData & BIT0 ? "Page-Protection violation" : "Non-Present page");
            DebugPrint(0, "Mode: %a\n", ctx->ExceptionData & BIT2 ? "User" : "Supervisor");
            DebugPrint(0, "Address: %p\n", AsmReadCr2());
            if(ctx->ExceptionData & BIT4) {
                DebugPrint(0, "Operation: Instruction Fetch\n");
            }else if(ctx->ExceptionData & BIT3) {
                DebugPrint(0, "Operation: Reserved Write\n");
            }else {
                DebugPrint(0, "Operation: %a\n", ctx->ExceptionData & BIT1 ? "Write" : "Read");
            }
        } break;

        // no exception data
        default:
            if(ctx->ExceptionData != 0) {
                DebugPrint(0, "Exception data: %d\n", ctx->ExceptionData);
            }
            break;
    }

    // prepare some bits
    IA32_EFLAGS32 flags = { .UintN = ctx->Rflags };
    char _cf = (char) ((flags.Bits.CF) != 0 ? 'C' : '-');
    char _pf = (char) ((flags.Bits.PF) != 0 ? 'P' : '-');
    char _af = (char) ((flags.Bits.AF) != 0 ? 'A' : '-');
    char _zf = (char) ((flags.Bits.ZF) != 0 ? 'Z' : '-');
    char _sf = (char) ((flags.Bits.SF) != 0 ? 'S' : '-');
    char _tf = (char) ((flags.Bits.TF) != 0 ? 'T' : '-');
    char _if = (char) ((flags.Bits.IF) != 0 ? 'I' : '-');
    int cpl = flags.Bits.IOPL;

    // print out the full state
    DebugPrint(0, "RAX=%016llx RBX=%016llx\nRCX=%016llx RDX=%016llx\n", ctx->Rax, ctx->Rbx, ctx->Rcx, ctx->Rdx);
    DebugPrint(0, "RSI=%016llx RDI=%016llx\nRBP=%016llx RSP=%016llx\n", ctx->Rsi, ctx->Rdi, ctx->Rbp, ctx->Rsp);
    DebugPrint(0, "R8 =%016llx R9 =%016llx\nR10=%016llx R11=%016llx\n", ctx->R8, ctx->R9, ctx->R10, ctx->R10);
    DebugPrint(0, "R12=%016llx R13=%016llx\nR14=%016llx R15=%016llx\n", ctx->R12, ctx->R13, ctx->R14, ctx->R15);
    DebugPrint(0, "RIP=%016llx RFL=%08lx [%c%c%c%c%c%c%c] CPL=%d\n", ctx->Rip, (uint32_t)ctx->Rflags, _if, _tf, _sf, _zf, _af, _pf, _cf, cpl);
    DebugPrint(0, "ES =%04x DPL=%d       ", ctx->Es & 0xFFF8, ctx->Es & 0b11);
    DebugPrint(0, "CS =%04x DPL=%d\n", ctx->Cs & 0xFFF8, ctx->Cs & 0b11);
    DebugPrint(0, "SS =%04x DPL=%d       ", ctx->Ss & 0xFFF8, ctx->Ss & 0b11);
    DebugPrint(0, "DS =%04x DPL=%d\n", ctx->Ds & 0xFFF8, ctx->Ds & 0b11);
    DebugPrint(0, "FS =%04x DPL=%d       ", ctx->Fs & 0xFFF8, ctx->Fs & 0b11);
    DebugPrint(0, "GS =%04x DPL=%d\n", ctx->Gs & 0xFFF8, ctx->Gs & 0b11);
    DebugPrint(0, "LDT=%04x DPL=%d       ", ctx->Ldtr & 0xFFF8, ctx->Ldtr & 0b11);
    DebugPrint(0, "TR =%04x DPL=%d\n", ctx->Tr & 0xFFF8, ctx->Tr & 0b11);
    DebugPrint(0, "GDT=%016llx %08llx\n", ctx->Gdtr[0], ctx->Gdtr[1]);
    DebugPrint(0, "IDT=%016llx %08llx\n", ctx->Idtr[0], ctx->Idtr[1]);
    DebugPrint(0, "CR0=%08x CR2=%016llx\nCR3=%016llx CR4=%08x\n", ctx->Cr0, ctx->Cr2, ctx->Cr3, ctx->Cr4);

    // TODO: debug registers
    // TODO: fpu registers

    DebugPrint(0, ":(");

    // and die
    CpuDeadLoop();
}

static void setup_exception_handlers() {
    EFI_CPU_ARCH_PROTOCOL* cpuArch = NULL;
    if(EFI_ERROR(gBS->LocateProtocol(&gEfiCpuArchProtocolGuid, NULL, (void**)&cpuArch))) return;
    if(!cpuArch) return;
    DebugPrint(0, "Cpu arch found, setting exception handlers\n");

    /*
     * The bios might set some handlers by itself, but we are going to cross our fingers that
     * these are not too important and try to remove theirs and install ours, but we are not
     * gonna assert just so we won't cause any problem with booting since this is just a debugging
     * feature anyways
     *
     * we are not going to try and unregister the NMI and Machine Check exceptions because these
     * are stuff that might be actually needed by the bios to act correctly.
     *
     * just cross fingers they don't use page faults or something alike :shrug:
     */
    for(int i = 0; i < ARRAY_SIZE(EXCEPTIONS); i++) {
        if(EXCEPTIONS[i] != EXCEPT_IA32_NMI && EXCEPTIONS[i] != EXCEPT_IA32_MACHINE_CHECK) {
            cpuArch->RegisterInterruptHandler(cpuArch, EXCEPTIONS[i], NULL);
        }

        cpuArch->RegisterInterruptHandler(cpuArch, EXCEPTIONS[i], interrupt_handler);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The entry
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EFI_STATUS EFIAPI EfiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable) {
    // setup some stuff
    gImageHandle = ImageHandle;
    gST = SystemTable;
    gRT = gST->RuntimeServices;
    gBS = gST->BootServices;

    // setup all of the libraries
    DebugLibConstructor(ImageHandle, SystemTable);
    setup_exception_handlers();

    // load the boot config
    get_boot_entries(&boot_entries);

    // start the menu thingy
    start_menus();

    // failed to load the kernel
    return EFI_LOAD_ERROR;
}
