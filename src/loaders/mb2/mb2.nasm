[BITS 64]
[DEFAULT REL]
[SECTION .text]

[GLOBAL JumpToMB2Kernel]
JumpToMB2Kernel:
    mov rbx, rdx
    push rcx

    ; Jump into the compatibility mode CS
    push 0x10
    lea rax, [.comp_mode]
    push rax
    DB 0x48, 0xcb ; retfq

[BITS 32]
    .comp_mode:
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

        ; Disable PAE
        mov eax, cr4
        btc eax, 5
        mov cr4, eax

        ; Setup bootloader magic and jump to kernel
        mov eax, 0x36d76289
        ret