[BITS 64]
[DEFAULT REL]
[SECTION .text]

[GLOBAL JumpToStivaleKernel]
JumpToStivaleKernel:
    mov rdi, rcx
    mov rsp, rdx
    jmp r8
