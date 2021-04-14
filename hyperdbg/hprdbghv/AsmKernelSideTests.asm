PUBLIC AsmTestExAllocatePoolWithTag

EXTERN ExAllocatePoolWithTag:PROC

.code _text

;------------------------------------------------------------------------

AsmTestExAllocatePoolWithTag PROC PUBLIC


    push r15			; Save the register we want to change
    push r14
    push r13
    push r12
    push r11
    push r10            ; rcx, rdx, r8, r9 should not be changed
                        ; due to the fast calling conventions

    mov r15, 0123h      ; Move new values to the registers
    mov r14, 0123h
    mov r13, 0123h
    mov r12, 0123h
    mov r11, 0123h
    mov r10, 0123h

    sub rsp, 040h       ; Shadow space
    
    ;call ExAllocatePoolWithTag      ; Call target function (we didn't change it's parameters)

    add rsp, 040h       ; Shadow space

    pop r10             ; Restore previous state
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

    ret

AsmTestExAllocatePoolWithTag ENDP


;------------------------------------------------------------------------

END
