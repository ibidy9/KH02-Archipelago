/* Original executes first. Validator returns original RAX or clears AL only. */
.text
.globl kh02_menu_thunk
.seh_proc kh02_menu_thunk
kh02_menu_thunk:
    subq $216, %rsp
    .seh_stackalloc 216
    .seh_endprologue
    movq %rcx, 80(%rsp)
    movq %rdx, 88(%rsp)
    movq %r8, 96(%rsp)
    call *kh02_menu_trampoline(%rip)
    movq %rax, 128(%rsp)
    movdqu %xmm0, 144(%rsp)
    movq 80(%rsp), %rcx
    movq 88(%rsp), %rdx
    movq 96(%rsp), %r8
    movq 216(%rsp), %r9
    movq %rax, 32(%rsp)
    call kh02_menu_observe
    movdqu 144(%rsp), %xmm0
    addq $216, %rsp
    ret
.seh_endproc
