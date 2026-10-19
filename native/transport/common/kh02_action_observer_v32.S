.text
.globl kh02_default_ability_thunk
.seh_proc kh02_default_ability_thunk
kh02_default_ability_thunk:
    pushfq
    .seh_stackalloc 8
    subq $192, %rsp
    .seh_stackalloc 192
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
    movq 200(%rsp), %r8
    movl $1, %edx
    movl $0, %r9d
    call kh02_default_ability_capture
    testl %eax, %eax
    jnz kh02_default_ability_blocked
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
    addq $192, %rsp
    popfq
    jmp *kh02_default_ability_trampoline(%rip)
kh02_default_ability_blocked:
    movq $0, 32(%rsp)
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
    addq $192, %rsp
    popfq
    ret
.seh_endproc

.text
.globl kh02_native_ability_thunk
.seh_proc kh02_native_ability_thunk
kh02_native_ability_thunk:
    pushfq
    .seh_stackalloc 8
    subq $192, %rsp
    .seh_stackalloc 192
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
    movq 200(%rsp), %r8
    movl $1, %r9d
    call kh02_default_ability_capture
    testl %eax, %eax
    jnz kh02_native_ability_blocked
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
    addq $192, %rsp
    popfq
    jmp *kh02_native_ability_trampoline(%rip)
kh02_native_ability_blocked:
    movq $0, 32(%rsp)
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
    addq $192, %rsp
    popfq
    ret
.seh_endproc

.text
.globl kh02_guard_thunk
.seh_proc kh02_guard_thunk
kh02_guard_thunk:
    pushfq
    .seh_stackalloc 8
    subq $192, %rsp
    .seh_stackalloc 192
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
    movq 200(%rsp), %r8
    movl $2, %edx
    movl $2, %r9d
    call kh02_guard_dodge_capture
    testl %eax, %eax
    jnz kh02_guard_blocked
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
    addq $192, %rsp
    popfq
    jmp *kh02_guard_trampoline(%rip)
kh02_guard_blocked:
    movq $0, 32(%rsp)
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
    addq $192, %rsp
    popfq
    ret
.seh_endproc

.text
.globl kh02_dodge_thunk
.seh_proc kh02_dodge_thunk
kh02_dodge_thunk:
    pushfq
    .seh_stackalloc 8
    subq $192, %rsp
    .seh_stackalloc 192
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
    movq 200(%rsp), %r8
    movl $28, %edx
    movl $3, %r9d
    call kh02_guard_dodge_capture
    testl %eax, %eax
    jnz kh02_dodge_blocked
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
    addq $192, %rsp
    popfq
    jmp *kh02_dodge_trampoline(%rip)
kh02_dodge_blocked:
    movq $0, 32(%rsp)
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
    addq $192, %rsp
    popfq
    ret
.seh_endproc

.text
.globl kh02_magic_predicate_thunk
.seh_proc kh02_magic_predicate_thunk
kh02_magic_predicate_thunk:
    pushfq
    .seh_stackalloc 8
    subq $192, %rsp
    .seh_stackalloc 192
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
    movq 200(%rsp), %r8
    call kh02_magic_predicate_capture
    testl %eax, %eax
    jnz kh02_magic_predicate_blocked
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
    addq $192, %rsp
    popfq
    jmp *kh02_magic_predicate_trampoline(%rip)
kh02_magic_predicate_blocked:
    movq $0, 32(%rsp)
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
    addq $192, %rsp
    popfq
    ret
.seh_endproc
