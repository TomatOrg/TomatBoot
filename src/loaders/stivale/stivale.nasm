[BITS 64]
[DEFAULT REL]
[SECTION .text]

[GLOBAL JumpToStivaleKernel]
JumpToStivaleKernel:
    mov rdi, rdx
    mov rsp, rcx
    jmp r8
