#include <debug/exception.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#define EXCEPT_IA32_DIVIDE_ERROR    0
#define EXCEPT_IA32_DEBUG           1
#define EXCEPT_IA32_NMI             2
#define EXCEPT_IA32_BREAKPOINT      3
#define EXCEPT_IA32_OVERFLOW        4
#define EXCEPT_IA32_BOUND           5
#define EXCEPT_IA32_INVALID_OPCODE  6
#define EXCEPT_IA32_DOUBLE_FAULT    8
#define EXCEPT_IA32_INVALID_TSS     10
#define EXCEPT_IA32_SEG_NOT_PRESENT 11
#define EXCEPT_IA32_STACK_FAULT     12
#define EXCEPT_IA32_GP_FAULT        13
#define EXCEPT_IA32_PAGE_FAULT      14
#define EXCEPT_IA32_FP_ERROR        16
#define EXCEPT_IA32_ALIGNMENT_CHECK 17
#define EXCEPT_IA32_MACHINE_CHECK   18
#define EXCEPT_IA32_SIMD            19

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

static void register_handler(void* handler, int vector, int override);
static struct idt_entry_t idt[256];
static struct idt_entry_t* default_idt = 0;

extern void div0_handler();
extern void debug_handler();
extern void breakpoint_handler();
extern void overflow_handler();
extern void bound_range_handler();
extern void invalid_opcode_handler();
extern void device_not_avail_handler();
extern void double_fault_handler();
extern void invalid_tss_handler();
extern void segment_not_present_handler();
extern void stack_segment_fault_handler();
extern void gpf_handler();
extern void pf_handler();
extern void x87_fp_exception_handler();
extern void alignment_check_handler();
extern void simd_fp_exception_handler();

//Override all idt entries with our own handler.
void hook_idt() {
    if(!default_idt) {
        struct idt_ptr_t old_idt_ptr = {0};
        __asm__ volatile (
            "sidt %0"
            :
            : "m" (old_idt_ptr)
        );

        default_idt = (struct idt_entry_t*)old_idt_ptr.address;
        //Copy all of the old interrupt handlers.
        for(int i = 0; i < 256; i++) {
            idt[i] = default_idt[i];
        }
    }
  
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
    register_handler(div0_handler, 0x0, 1);
    register_handler(debug_handler, 0x1, 1);
    register_handler(breakpoint_handler, 0x3, 1);
    register_handler(overflow_handler, 0x4, 1);
    register_handler(bound_range_handler, 0x5, 1);
    register_handler(invalid_opcode_handler, 0x6, 1);
    register_handler(device_not_avail_handler, 0x7, 1);
    register_handler(double_fault_handler, 0x8, 1);
    register_handler(invalid_tss_handler, 0xA, 1);
    register_handler(segment_not_present_handler, 0xB, 1);
    register_handler(stack_segment_fault_handler, 0xC, 1);
    register_handler(gpf_handler, 0xD,1);
    register_handler(pf_handler, 0xE, 1);
    register_handler(x87_fp_exception_handler, 0x10, 1);
    register_handler(alignment_check_handler, 0x11, 1);
    register_handler(simd_fp_exception_handler, 0x13, 1);

    struct idt_ptr_t idt_ptr = {
        sizeof(idt) - 1,
        (UINT64)idt
    };

    __asm__ volatile (
        "lidt %0"
        :
        : "m" (idt_ptr)
    );
}

void unhook_idt() {
    //Copy all of the old interrupt handlers.
    for(int i = 0; i < 256; i++) {
        idt[i] = default_idt[i];
    }
}  
 

static void register_handler(void* handler, int vector, int override) {
    //Check if the entry is already present.
    if(!override) {
        if(idt[vector].type_attr & (1 << 7)) {
            return;
        }
    }

    UINT16 cs = 0;

    __asm__ volatile (
        "mov %%cs, %0"
        :
        : "m" (cs)
    );

    idt[vector].offset_low = (UINT16)handler;
    idt[vector].selector = cs;
    idt[vector].ist = 0;
    //Present, DPL 0, Storage Segment 0, trap gate.
    idt[vector].type_attr = 0x8e;
    idt[vector].offset_mid = (UINT16)((size_t)handler >> 16);
    idt[vector].offset_high = (UINT32)((size_t)handler >> 32);
    idt[vector].zero = 0;
}
void interrupt_handler(const unsigned int InterruptType, const struct regs_t* ctx) {
    // print the exception itself
    if(InterruptType < ARRAY_SIZE(EXCEPTION_NAMES) && EXCEPTION_NAMES[InterruptType] != 0) {
        DebugPrint(0, "Exception: %a\n", EXCEPTION_NAMES[InterruptType]);
    }else {
        DebugPrint(0, "Exception: %d\n", InterruptType);
    }
    DebugPrint(0, "function addr: %p\n", interrupt_handler);

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
    DebugPrint(0, "RIP=%016llx RFL=%08lx [%c%c%c%c%c%c%c] CPL=%d\n", ctx->Rip, (UINT32)ctx->Rflags, _if, _tf, _sf, _zf, _af, _pf, _cf, cpl);
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

