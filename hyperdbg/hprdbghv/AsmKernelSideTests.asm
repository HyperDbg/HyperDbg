PUBLIC AsmTestWrapperWithTestTags

EXTERN g_KernelTestTargetFunction:QWORD
EXTERN g_KernelTestTag1:QWORD
EXTERN g_KernelTestTag2:QWORD

EXTERN g_KernelTestR15:QWORD
EXTERN g_KernelTestR14:QWORD
EXTERN g_KernelTestR13:QWORD
EXTERN g_KernelTestR12:QWORD

.code _text

;------------------------------------------------------------------------

AsmTestWrapperWithTestTags PROC PUBLIC

    ; rcx, rdx, r8, r9 should not be changed due to the fast calling conventions

    ; Generally, The registers RAX, RCX, RDX, R8, R9, R10, R11 are
    ; considered volatile (caller-saved) and registers RBX, RBP, RDI, 
    ; RSI, RSP, R12, R13, R14, and R15 are considered non-volatile (callee-saved)

    ; save registers 
    mov g_KernelTestR12, r12       ; save r12
    mov g_KernelTestR13, r13       ; save r13
    mov g_KernelTestR14, r14       ; save r14
    mov g_KernelTestR15, r15       ; save r15

    ; apply tags to r15, and r14
    mov r14, g_KernelTestTag1
    mov r15, g_KernelTestTag2

    pop r12                        ; r12 is not changed (non-volatile)
                                   ; we save it so we can restore to 
                                   ; TestKernelConfigureTagsAndCallTargetFunction

    mov r13, RestoreState          ; use r13 to simulate the return point
    push r13
    
    jmp g_KernelTestTargetFunction      ; jump target function (we didn't change its parameters)

RestoreState:
    push r12                       ; Restore to the previous function address to return
                                   ; TestKernelConfigureTagsAndCallTargetFunction
    
    ; restore registers 
    mov r12, g_KernelTestR12        ; restore r12
    mov r13, g_KernelTestR13        ; restore r13
    mov r14, g_KernelTestR14        ; restore r14
    mov r15, g_KernelTestR15        ; restore r15

    ret

AsmTestWrapperWithTestTags ENDP

;------------------------------------------------------------------------

END
