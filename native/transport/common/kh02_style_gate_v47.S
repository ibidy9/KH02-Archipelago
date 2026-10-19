/* Void request function: preserve its integer and floating-point arguments. */
.text
.globl kh02_style_request_thunk
.seh_proc kh02_style_request_thunk
kh02_style_request_thunk:
    subq $136, %rsp
    .seh_stackalloc 136
    .seh_endprologue
    movq %rcx, 32(%rsp)
    movq %rdx, 40(%rsp)
    movq %r8, 48(%rsp)
    movq %r9, 56(%rsp)
    movdqu %xmm0, 64(%rsp)
    movdqu %xmm1, 80(%rsp)
    movdqu %xmm2, 96(%rsp)
    movdqu %xmm3, 112(%rsp)
    movq 136(%rsp), %r8
    call kh02_style_request_gate
    testl %eax, %eax
    jnz .Lstyle_blocked
    movq 32(%rsp), %rcx
    movq 40(%rsp), %rdx
    movq 48(%rsp), %r8
    movq 56(%rsp), %r9
    movdqu 64(%rsp), %xmm0
    movdqu 80(%rsp), %xmm1
    movdqu 96(%rsp), %xmm2
    movdqu 112(%rsp), %xmm3
    addq $136, %rsp
    jmp *kh02_style_request_trampoline(%rip)
.Lstyle_blocked:
    addq $136, %rsp
    ret
.seh_endproc
