PUBLIC AsmGeneralDetourHook
PUBLIC AsmDebuggerSpinOnThread
EXTERN ExAllocatePoolWithTagOrig:QWORD
EXTERN DebuggerEventEptHook2GeneralDetourEventHandler:PROC

.code _text

;------------------------------------------------------------------------

AsmGeneralDetourHook PROC PUBLIC

SaveTheRegisters:
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8        
    push rdi
    push rsi
    push rbp
    push rbp	; rsp
    push rbx
    push rdx
    push rcx
    push rax	

	mov rcx, rsp		    ; Fast call argument to PGUEST_REGS
    mov rdx, [rsp +080h]    ; Fast call argument (second) - CalledFrom
    sub rdx, 5              ; as we used (call $ + 5) so we subtract it by 5 
	sub	rsp, 28h		; Free some space for Shadow Section

	call	DebuggerEventEptHook2GeneralDetourEventHandler

	add	rsp, 28h		; Restore the state

    mov  [rsp +080h], rax ; the return address of the above function is where we should continue

RestoreTheRegisters:
	pop rax
    pop rcx
    pop rdx
    pop rbx
    pop rbp		; rsp
    pop rbp
    pop rsi
    pop rdi 
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
    ret ; jump back to the trampoline
    
AsmGeneralDetourHook ENDP 

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
    call R8 ; Because R8 contains the 4th argument and a pointer to the function of target condition code

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
    
    ;;;;; DO NOT CHANGE THE NOPS, THIS FUNCTION'S SIZE IS 7 BYTES (WITHOUT INT 3 AND RET)
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
