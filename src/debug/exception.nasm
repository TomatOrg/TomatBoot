global div0_handler
global debug_handler
global breakpoint_handler
global overflow_handler
global bound_range_handler
global invalid_opcode_handler
global device_not_avail_handler
global double_fault_handler
global invalid_tss_handler
global segment_not_present_handler
global stack_segment_fault_handler
global gpf_handler
global pf_handler
global x87_fp_exception_handler
global alignment_check_handler
global simd_fp_exception_handler
section .text

%macro common_handler 1
cld
push rax
push rbx
push rcx
push rdx
push rsi
push rdi
push rbp
push r8
push r9
push r10
push r11
push r12
push r13
push r14
push r15
mov  rax, ds
push rax
mov  rax, es
push rax
mov  rax, fs
push rax
mov  rax, gs
push rax
; Push GDT register and IDT register
xor  rax, rax
push rax
push rax
sidt [rsp]

mov     bx, word [rsp]
mov     rax, qword [rsp + 2]
mov     qword [rsp], rax
mov     word [rsp + 8], bx

xor rax, rax
push rax
push rax
sgdt [rsp]
mov     bx, word [rsp]
mov     rax, qword [rsp + 2]
mov     qword [rsp], rax
mov     word [rsp + 8], bx
xor rax, rax

; Tr and Ldtr
str  ax
push rax
sldt ax
push rax
; Control registers
mov  rax, cr4
push rax
mov  rax, cr3
push rax
mov  rax, cr2
push rax
xor  rax, rax
mov  rax, cr0
push rax

mov rcx, %1
mov rdx, rsp
; Shadow space on the stack
; required by the Microsoft x64 calling convention.
sub rsp, 4 * 8 + 8
extern interrupt_handler
call interrupt_handler
hlt
%endmacro

%macro exception_handler 1
push 0
common_handler %1
%endmacro

%macro exception_handler_error 1
common_handler %1
%endmacro

div0_handler:
    exception_handler 0x0
debug_handler:
    exception_handler 0x1
breakpoint_handler:
    exception_handler 0x3
overflow_handler:
    exception_handler 0x4
bound_range_handler:
    exception_handler 0x5
invalid_opcode_handler:
    exception_handler 0x6
device_not_avail_handler:
    exception_handler 0x7
double_fault_handler:
    exception_handler_error 0x8
invalid_tss_handler:
    exception_handler_error 0xA
segment_not_present_handler:
    exception_handler_error 0xB
stack_segment_fault_handler:
    exception_handler_error 0xC
gpf_handler:
    exception_handler_error 0xD
pf_handler:
    exception_handler_error 0xE
x87_fp_exception_handler:
    exception_handler 0x10
alignment_check_handler:
    exception_handler_error 0x11
simd_fp_exception_handler:
    exception_handler 0x13
