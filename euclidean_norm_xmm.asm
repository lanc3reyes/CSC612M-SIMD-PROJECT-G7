bits 64
default rel

section .data
    zero_xmm dq 0.0, 0.0

section .text
global euclidean_norm_xmm

euclidean_norm_xmm:
    vmovupd xmm0, [zero_xmm]

xmm_loop:
    cmp ecx, 2
    jl xmm_reduce

    vmovupd xmm1, [rdx]
    vmulpd xmm1, xmm1, xmm1
    vaddpd xmm0, xmm0, xmm1

    add rdx, 16
    sub ecx, 2
    jmp xmm_loop

xmm_reduce:
    vhaddpd xmm0, xmm0, xmm0

    cmp ecx, 0
    jle xmm_finish

    movsd xmm1, [rdx]
    mulsd xmm1, xmm1
    addsd xmm0, xmm1

xmm_finish:
    sqrtsd xmm0, xmm0
    ret
