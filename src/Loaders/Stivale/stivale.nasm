[BITS 64]

[DEFAULT REL]
[SECTION .text]
[GLOBAL StivaleSpinup]
StivaleSpinup:
    ; set the new stack and the stivale struct
    ; pointer properly
    mov rdi, rcx
    mov rsp, rdx

    ; check if we need to switch to 5
    ; level page tables
    test r9, r9
    jne Translate5Level

    ; no need, jump directly to kernel
    jmp bit64

Translate5Level:
    ; load our gdt which has compat mode entry
    mov rbx, rdx
    push rcx
    lgdt [gdt_ptr]
    lea rbx, [bit64]

    ; Jump into the compatibility mode CS
    push 0x10
    lea rax, [.cmp_mode]
    push rax
    DB 0x48, 0xcb ; retfq

[BITS 32]
.cmp_mode:

    ; Now in compatibility mode.
    mov ax, 0x18
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Disable paging
    mov eax, cr0
    btc eax, 31
    mov cr0, eax

    ; Disable long mode in EFER
    mov ecx, 0x0c0000080
    rdmsr
    btc eax, 8
    wrmsr

    ; Enable 5 level paging
    mov eax, cr4
    bts eax, 12
    mov cr4, eax

    ; Enable long mode in EFER
    mov ecx, 0x0c0000080
    rdmsr
    bts eax, 8
    wrmsr

    ; Copy it from the cr2 register to the
    ; cr3 register
    mov eax, cr2
    mov cr3, eax

    ; Enable paging
    mov eax, cr0
    bts eax, 31
    mov cr0, eax

    ; Reload cs
    push 0x8
    push ebx
    retf

[BITS 64]
bit64:
    ; reset all other regs
    xor rax, rax
    xor rbx, rbx
    xor rcx, rcx
    xor rdx, rdx
    xor rsi, rsi
    ; TODO: reset r8
    xor r9, r9
    xor r10, r10
    xor r11, r11
    xor r12, r12
    xor r13, r13
    xor r14, r14
    xor r15, r15

    ; Emulate a jump with 0 return address
    push 0
    jmp r8

[SECTION .data]
align 16

gdt_ptr:
    dw .gdt_end - .gdt_start - 1 ; gdt limit
    dq .gdt_start                ; gdt base

align 16
.gdt_start:
.null:
    dw 0x0000           ; limit low
    dw 0x0000           ; base low
    db 0x00             ; base mid
    dw 00000000b        ; flags
    db 0x00             ; base high

; 64 bit mode kernel
.kernel_code_64:
    dw 0x0000           ; limit low
    dw 0x0000           ; base low
    db 0x00             ; base mid
    dw 0x209A           ; flags
    db 0x00             ; base high

; 32 bit compat mode
.kernel_code_32:
    dw 0xFFFF           ; limit low
    dw 0x0000           ; base low
    db 0x00             ; base mid
    dw 0xCF9A           ; flags
    db 0x00             ; base high

.kernel_data:
    dw 0x0000           ; limit low
    dw 0x0000           ; base low
    db 0x00             ; base mid
    dw 0x92             ; flags
    db 0x00             ; base high
.gdt_end: