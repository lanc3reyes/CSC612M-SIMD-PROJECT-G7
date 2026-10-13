bits 64
default rel

section .data
    zero dq 0.0

section .text
global euclidean_norm_asm

euclidean_norm_asm:
    ; start running sum at zero
    movsd xmm0, [zero]
    cmp ecx, 0
    jle finish

scalar_loop:
    movsd xmm1, [rdx]
    mulsd xmm1, xmm1
    addsd xmm0, xmm1
    add rdx, 8

    dec ecx
    cmp ecx, 0
    jnz scalar_loop

finish:
    sqrtsd xmm0, xmm0
    ret


