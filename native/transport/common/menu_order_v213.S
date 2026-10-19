.text
.globl kh02_menu_order_thunk213
.seh_proc kh02_menu_order_thunk213
kh02_menu_order_thunk213:
    pushfq
    .seh_stackalloc 8
    subq $200, %rsp
    .seh_stackalloc 200
    .seh_endprologue
    movq %rax, 32(%rsp)
    movq %rcx, 40(%rsp)
    movq %rdx, 48(%rsp)
    movq %r8, 56(%rsp)
    movq %r9, 64(%rsp)
    movq %r10, 72(%rsp)
    movq %r11, 80(%rsp)
    movdqu %xmm0, 96(%rsp)
    movdqu %xmm1, 112(%rsp)
    movdqu %xmm2, 128(%rsp)
    movdqu %xmm3, 144(%rsp)
    movdqu %xmm4, 160(%rsp)
    movdqu %xmm5, 176(%rsp)
    movq %rdi, %rcx
    call kh02_menu_order_capture213
    movdqu 96(%rsp), %xmm0
    movdqu 112(%rsp), %xmm1
    movdqu 128(%rsp), %xmm2
    movdqu 144(%rsp), %xmm3
    movdqu 160(%rsp), %xmm4
    movdqu 176(%rsp), %xmm5
    movq 32(%rsp), %rax
    movq 40(%rsp), %rcx
    movq 48(%rsp), %rdx
    movq 56(%rsp), %r8
    movq 64(%rsp), %r9
    movq 72(%rsp), %r10
    movq 80(%rsp), %r11
    addq $200, %rsp
    popfq
    jmp *kh02_menu_order_trampoline213(%rip)
.seh_endproc

.text
.globl kh02_menu_seen_thunk213
.seh_proc kh02_menu_seen_thunk213
kh02_menu_seen_thunk213:
    pushfq
    .seh_stackalloc 8
    subq $200, %rsp
    .seh_stackalloc 200
    .seh_endprologue
    movq %rax, 32(%rsp)
    movq %rcx, 40(%rsp)
    movq %rdx, 48(%rsp)
    movq %r8, 56(%rsp)
    movq %r9, 64(%rsp)
    movq %r10, 72(%rsp)
    movq %r11, 80(%rsp)
    movdqu %xmm0, 96(%rsp)
    movdqu %xmm1, 112(%rsp)
    movdqu %xmm2, 128(%rsp)
    movdqu %xmm3, 144(%rsp)
    movdqu %xmm4, 160(%rsp)
    movdqu %xmm5, 176(%rsp)
    movq %rbx, %rcx
    leaq 0x3d18(%rax), %rdx
    movl %edi, %r8d
    call kh02_menu_seen_capture213
    movdqu 96(%rsp), %xmm0
    movdqu 112(%rsp), %xmm1
    movdqu 128(%rsp), %xmm2
    movdqu 144(%rsp), %xmm3
    movdqu 160(%rsp), %xmm4
    movdqu 176(%rsp), %xmm5
    movq 32(%rsp), %rax
    movq 40(%rsp), %rcx
    movq 48(%rsp), %rdx
    movq 56(%rsp), %r8
    movq 64(%rsp), %r9
    movq 72(%rsp), %r10
    movq 80(%rsp), %r11
    addq $200, %rsp
    popfq
    jmp *kh02_menu_seen_resume213(%rip)
.seh_endproc
