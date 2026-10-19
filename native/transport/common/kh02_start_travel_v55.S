.text
.globl kh02_travel_thunk
.seh_proc kh02_travel_thunk
kh02_travel_thunk:
    pushfq
    .seh_stackalloc 8
    subq $192, %rsp
    .seh_stackalloc 192
    .seh_endprologue
    movq %rax, 88(%rsp)
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
    leaq 40(%rsp), %rcx
    movq 200(%rsp), %rdx
    call kh02_original_route_v127
    leaq 40(%rsp), %rcx
    movq 200(%rsp), %rdx
    call kh02_goal_route_filter
    testl %eax, %eax
    jnz .Lgoal_refused
    leaq 40(%rsp), %rcx
    movq 200(%rsp), %rdx
    call kh02_start_route_adapt
    leaq 40(%rsp), %rcx
    movq 200(%rsp), %rdx
    call kh02_menu_depths_v146
    testl %eax, %eax
    jnz .Lgoal_refused
    movq 40(%rsp), %rcx
    movq 48(%rsp), %rdx
    movq 56(%rsp), %r8
    movq 64(%rsp), %r9
    movq 200(%rsp), %rax
    movq %rax, 32(%rsp)
    call kh02_travel_capture
    movdqu 96(%rsp), %xmm0
    movdqu 112(%rsp), %xmm1
    movdqu 128(%rsp), %xmm2
    movdqu 144(%rsp), %xmm3
    movdqu 160(%rsp), %xmm4
    movdqu 176(%rsp), %xmm5
    movq 88(%rsp), %rax
    movq 40(%rsp), %rcx
    movq 48(%rsp), %rdx
    movq 56(%rsp), %r8
    movq 64(%rsp), %r9
    movq 72(%rsp), %r10
    movq 80(%rsp), %r11
    addq $192, %rsp
    popfq
    jmp *kh02_travel_trampoline(%rip)
 .Lgoal_refused:
    movdqu 96(%rsp), %xmm0
    movdqu 112(%rsp), %xmm1
    movdqu 128(%rsp), %xmm2
    movdqu 144(%rsp), %xmm3
    movdqu 160(%rsp), %xmm4
    movdqu 176(%rsp), %xmm5
    movq 88(%rsp), %rax
    movq 40(%rsp), %rcx
    movq 48(%rsp), %rdx
    movq 56(%rsp), %r8
    movq 64(%rsp), %r9
    movq 72(%rsp), %r10
    movq 80(%rsp), %r11
    addq $192, %rsp
    popfq
    xorl %eax, %eax
    ret
.seh_endproc

