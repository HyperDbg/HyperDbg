PUBLIC AsmVmxSaveState
PUBLIC AsmVmxRestoreState

EXTERN VmxVirtualizeCurrentSystem:PROC

.code _text

;------------------------------------------------------------------------

AsmVmxSaveState PROC

    push 0 ; add it because the alignment of the RSP when calling the target function
     ; should be aligned to 16 (otherwise cause performance issues)

    pushfq	; save r/eflag
    
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    
    sub rsp, 0100h
    ; It a x64 FastCall function so the first parameter should go to rcx
    
    mov rcx, rsp
    call VmxVirtualizeCurrentSystem
    
    int 3	; we should never reach here as we execute vmlaunch in the above function.
    		; if rax is FALSE then it's an indication of error
    
    jmp AsmVmxRestoreState
    	
AsmVmxSaveState ENDP

;------------------------------------------------------------------------

AsmVmxRestoreState PROC
    
    add rsp, 0100h
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax
    
    popfq	; restore r/eflags
    add rsp, 08h ; because we pushed an etra qword to make it aligned
    ret
    
AsmVmxRestoreState ENDP

;------------------------------------------------------------------------

END
