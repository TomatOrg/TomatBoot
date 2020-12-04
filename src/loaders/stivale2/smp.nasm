[SECTION .data]
[GLOBAL gSmpTrampoline]
gSmpTrampoline:
    [BITS 16]

    ; clear everything
    cli
    cld

    ; set the ds segment to be 0
    xor ax, ax
    mov ds, ax

    ; set the gdt
    lgdt [gSmpTplGdt]

    ; enter protected mode
    mov eax, cr0
    bts eax, 0
    mov cr0, eax

    ; set the cs segment
    jmp 0x18:.mode32
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

[GLOBAL gSmpTplBootedFlag]
gSmpTplBootedFlag:
    dd 0

[GLOBAL gSmpTplInfoStruct]
gSmpTplInfoStruct:
    dq 0

[GLOBAL gSmpTrampolineEnd]
gSmpTrampolineEnd:

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[GLOBAL gSmpTplGdt]
gSmpTplGdt:
    dw 0
    dd 0

[GLOBAL gSmpTplTargetMode]
gSmpTplTargetMode:
    dd 0

[GLOBAL gSmpTplPagemap]
gSmpTplPagemap:
    dd 0
