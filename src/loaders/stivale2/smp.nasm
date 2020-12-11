[SECTION .data]
[GLOBAL gSmpTrampoline]
gSmpTrampoline:
    [BITS 16]

    ; At this point CS = 0x(vv00) and ip= 0x0.

    ; clear everything
    cli
    cld

    ; set the gdt
    mov si, gSmpTplGdt - gSmpTrampoline
o32 lgdt [cs:si]

    ; calculate our physical address ((.mode32 - base) + cs * 16)
    mov edi, .mode32 - gSmpTrampoline
    mov ax, cs
    shl eax, 4
    add edi, eax

    ; set it in the buffer
    mov si, gMode32Addr - gSmpTrampoline
    mov [cs:si], edi

    ; enter protected mode
    mov eax, cr0
    bts eax, 0
    mov cr0, eax

    ; jump to it
    jmp far [cs:si]
[BITS 32]
    ; set all the segment registers
.mode32:
    mov ax, 0x20
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov eax, cr0
    btr eax, 29
    btr eax, 30
    mov cr0, eax

    ; check if we need to enable x2apic
    test dword [gSmpTplTargetMode], (1 << 2)
    jz .nox2apic

    ; configure x2apic
    mov ecx, 0x1b
    rdmsr
    bts eax, 10
    bts eax, 11
    wrmsr

.nox2apic:

    ;
    mov eax, cr4
    bts eax, 5
    mov cr4, eax

    ; check for 5 level paging
    test dword [gSmpTplTargetMode], (1 << 1)
    jz .no5lv

    ; enable 5 level paging
    mov eax, cr4
    bts eax, 12
    mov cr4, eax

.no5lv:
    ; set the pagetable
    mov eax, dword [gSmpTplPagemap]
    mov cr3, eax

    ; enable long mode
    mov ecx, 0xc0000080
    rdmsr
    bts eax, 8
    wrmsr

    ; enable paging
    mov eax, cr0
    bts eax, 31
    mov cr0, eax

    ; actually enter long mode
    jmp 0x28:.mode64
[BITS 64]
.mode64:
    ; set the data segments
    mov ax, 0x30
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; load the info struct and set the booted flag
    mov rdi, qword [rel gSmpTplInfoStruct]
    mov eax, 1
    lock xchg dword [rel gSmpTplBootedFlag], eax

    xor eax, eax
.loop:
    ; check if the flag was set
    lock xadd qword [rdi + 16], rax
    test rax, rax
    jnz .out

    ; no, pause and jump
    ; back to loop
    pause
    jmp .loop

.out:
    mov rsp, qword [rdi + 8]
    push 0
    push rax
    xor rax, rax
    xor rbx, rbx
    xor rcx, rcx
    xor rdx, rdx
    xor rsi, rsi
    xor rbp, rbp
    xor r8,  r8
    xor r9,  r9
    xor r10, r10
    xor r11, r11
    xor r12, r12
    xor r13, r13
    xor r14, r14
    xor r15, r15
    ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; These variables are accessed with relative addressing
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

gMode32Addr:
    dd 0x0000
    dw 24

[GLOBAL gSmpTplBootedFlag]
gSmpTplBootedFlag:
    dd 0

[GLOBAL gSmpTplInfoStruct]
gSmpTplInfoStruct:
    dq 0

[GLOBAL gSmpTplGdt]
gSmpTplGdt:
    dw 0
    dd 0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[GLOBAL gSmpTrampolineEnd]
gSmpTrampolineEnd:

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; these variables are accessed with abs addresses
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[GLOBAL gSmpTplTargetMode]
gSmpTplTargetMode:
    dd 0

[GLOBAL gSmpTplPagemap]
gSmpTplPagemap:
    dd 0

[GLOBAL gGdtPtr]
gGdtPtr:
    dw .size - 1    ; GDT size
    dd .start       ; GDT start address

    .start:
        ; Null desc
        dq 0

        ; 16-bit code
        dw 0xffff       ; Limit
        dw 0x0000       ; Base (low 16 bits)
        db 0x00         ; Base (mid 8 bits)
        db 10011010b    ; Access
        db 00000000b    ; Granularity
        db 0x00         ; Base (high 8 bits)

        ; 16-bit data
        dw 0xffff       ; Limit
        dw 0x0000       ; Base (low 16 bits)
        db 0x00         ; Base (mid 8 bits)
        db 10010010b    ; Access
        db 00000000b    ; Granularity
        db 0x00         ; Base (high 8 bits)

        ; 32-bit code
        dw 0xffff       ; Limit
        dw 0x0000       ; Base (low 16 bits)
        db 0x00         ; Base (mid 8 bits)
        db 10011010b    ; Access
        db 11001111b    ; Granularity
        db 0x00         ; Base (high 8 bits)

        ; 32-bit data
        dw 0xffff       ; Limit
        dw 0x0000       ; Base (low 16 bits)
        db 0x00         ; Base (mid 8 bits)
        db 10010010b    ; Access
        db 11001111b    ; Granularity
        db 0x00         ; Base (high 8 bits)

        ; 64-bit code
        dw 0x0000       ; Limit
        dw 0x0000       ; Base (low 16 bits)
        db 0x00         ; Base (mid 8 bits)
        db 10011010b    ; Access
        db 00100000b    ; Granularity
        db 0x00         ; Base (high 8 bits)

        ; 64-bit data
        dw 0x0000       ; Limit
        dw 0x0000       ; Base (low 16 bits)
        db 0x00         ; Base (mid 8 bits)
        db 10010010b    ; Access
        db 00000000b    ; Granularity
        db 0x00         ; Base (high 8 bits)
    .end:
    .size: equ .end - .start
