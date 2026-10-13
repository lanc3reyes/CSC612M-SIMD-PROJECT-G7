bits 64
default rel

section .data
    zero_ymm dq 0.0, 0.0, 0.0, 0.0

section .text
global euclidean_norm_ymm

; ymm0 holds four partial sums, then xmm0 holds the scalar result.
euclidean_norm_ymm:
    vmovupd ymm0, [zero_ymm]

ymm_loop:
    cmp ecx, 4
    jl ymm_reduce

    vmovupd ymm1, [rdx]
    vmulpd ymm1, ymm1, ymm1
    vaddpd ymm0, ymm0, ymm1
    add rdx, 32
    sub ecx, 4
    jmp ymm_loop

ymm_reduce:
    vextractf128 xmm1, ymm0, 1
    vaddpd xmm0, xmm0, xmm1
    vhaddpd xmm0, xmm0, xmm0

ymm_tail:
    cmp ecx, 0
    jle ymm_finish

    vmovsd xmm1, [rdx]
    vmulsd xmm1, xmm1, xmm1
    vaddsd xmm0, xmm0, xmm1
    add rdx, 8
    dec ecx
    jmp ymm_tail

ymm_finish:
    vsqrtsd xmm0, xmm0, xmm0
    vzeroupper
    ret