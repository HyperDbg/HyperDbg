PUBLIC AsmInvept

.code _text

;------------------------------------------------------------------------

; Error codes :

    VMX_ERROR_CODE_SUCCESS              = 0
    VMX_ERROR_CODE_FAILED_WITH_STATUS   = 1
    VMX_ERROR_CODE_FAILED               = 2

;------------------------------------------------------------------------

AsmInvept PROC PUBLIC

    invept  rcx, oword ptr [rdx]
    jz ErrorWithStatus
    jc ErrorCodeFailed
    
    xor     rax, rax
    ret

ErrorWithStatus: 
    mov     rax, VMX_ERROR_CODE_FAILED_WITH_STATUS
    ret

ErrorCodeFailed:
    mov     rax, VMX_ERROR_CODE_FAILED
    ret

AsmInvept ENDP

;------------------------------------------------------------------------

AsmInvvpid PROC

    invvpid rcx, oword ptr [rdx]
    jz      ErrorWithStatus
    jc      ErrorCodeFailed
    xor     rax, rax
    ret
    
ErrorWithStatus:
    mov     rax, VMX_ERROR_CODE_FAILED_WITH_STATUS
    ret

ErrorCodeFailed:
    mov     rax, VMX_ERROR_CODE_FAILED
    ret
    
AsmInvvpid ENDP

;------------------------------------------------------------------------

END
