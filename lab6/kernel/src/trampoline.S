.section .rodata
    .global sigreturn_stub
    .global sigreturn_stub_end

sigreturn_stub:
    li a7, 11                 # Syscall 11 (sigreturn)
    ecall
sigreturn_stub_end: