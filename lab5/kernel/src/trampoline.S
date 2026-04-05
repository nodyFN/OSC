.section .rodata
    .global sigreturn_stub
    .global sigreturn_stub_end

sigreturn_stub:
    li a7, 10                 # Syscall 10 (sigreturn)
    ecall
sigreturn_stub_end: