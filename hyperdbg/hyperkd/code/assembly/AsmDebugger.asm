PUBLIC AsmDebuggerCustomCodeHandler
PUBLIC AsmDebuggerConditionCodeHandler
PUBLIC AsmDebuggerSpinOnThread

.code _text

;------------------------------------------------------------------------

AsmDebuggerCustomCodeHandler PROC PUBLIC

; Generally, The registers RAX, RCX, RDX, R8, R9, R10, R11 are considered volatile (caller-saved) 
; and registers RBX, RBP, RDI, RSI, RSP, R12, R13, R14, and R15 are considered nonvolatile (callee-saved).


SaveTheRegisters:
    push RBX
    push RBP
    push RDI
    push RSI
    push R12
    push R13
    push R14
    push R15        	

    ; The function will be called as DebuggerRunCustomCodeFunc(PVOID PreAllocatedBufferAddress, PGUEST_REGS Regs, PVOID Context);
    call R9 ; Because R9 contains the 4th argument and a pointer to the function of target custom code

RestoreTheRegisters:
    pop R15 
    pop R14
    pop R13
    pop R12
    pop RSI
    pop RDI
    pop RBP
    pop RBX

    ret ; jump back to the trampoline
    
AsmDebuggerCustomCodeHandler ENDP 

;------------------------------------------------------------------------

AsmDebuggerConditionCodeHandler PROC PUBLIC

; Generally, The registers RAX, RCX, RDX, R8, R9, R10, R11 are considered volatile (caller-saved) 
; and registers RBX, RBP, RDI, RSI, RSP, R12, R13, R14, and R15 are considered nonvolatile (callee-saved).

SaveTheRegisters:
    push RBX
    push RBP
    push RDI
    push RSI
    push R12
    push R13
    push R14
    push R15        	

    ; The function will be called as DebuggerCheckForCondition(PGUEST_REGS Regs, PVOID Context);
    call R8 ; Because R8 contains the 3th argument and a pointer to the function of target condition code

RestoreTheRegisters:
    pop R15 
    pop R14
    pop R13
    pop R12
    pop RSI
    pop RDI
    pop RBP
    pop RBX

    ret ; jump back to the trampoline
    
AsmDebuggerConditionCodeHandler ENDP 

;------------------------------------------------------------------------

AsmDebuggerSpinOnThread PROC PUBLIC
    
    ; DO NOT CHANGE THE NOPS, THIS FUNCTION'S SIZE IS 7 BYTES (WITHOUT INT 3 AND RET)
NopLoop:
    nop
    nop
    nop
    nop
    nop
    jmp NopLoop

    int 3 ; we should never reach here

    ret 
    
AsmDebuggerSpinOnThread ENDP 

END                     
