.text
.globl kh02_menu_discovery_thunk151
.seh_proc kh02_menu_discovery_thunk151
kh02_menu_discovery_thunk151:
    subq $40, %rsp
    .seh_stackalloc 40
    .seh_endprologue
    movq %rsi, %r8
    movq 40(%rsp), %r9
    movl %r15d, 32(%rsp)
    call kh02_menu_discovery_capture151
    addq $40, %rsp
    ret
.seh_endproc
