PUBLIC AsmGetCs
PUBLIC AsmGetDs
PUBLIC AsmSetDs
PUBLIC AsmGetEs
PUBLIC AsmSetEs
PUBLIC AsmGetSs
PUBLIC AsmSetSs
PUBLIC AsmGetFs
PUBLIC AsmSetFs
PUBLIC AsmGetGs
PUBLIC AsmGetLdtr
PUBLIC AsmGetTr
PUBLIC AsmGetGdtBase
PUBLIC AsmGetIdtBase
PUBLIC AsmGetGdtLimit
PUBLIC AsmGetIdtLimit

.code _text

;------------------------------------------------------------------------

AsmGetGdtBase PROC

    LOCAL   gdtr[10]:BYTE
    sgdt    gdtr
    mov     rax, QWORD PTR gdtr[2]
    ret

AsmGetGdtBase ENDP

;------------------------------------------------------------------------

AsmGetCs PROC

    mov     rax, cs
    ret

AsmGetCs ENDP

;------------------------------------------------------------------------

AsmGetDs PROC

    mov     rax, ds
    ret

AsmGetDs ENDP

;------------------------------------------------------------------------

AsmSetDs PROC

    mov     rax, rcx
    mov     ds, rax 
    ret

AsmSetDs ENDP

;------------------------------------------------------------------------

AsmGetEs PROC

    mov     rax, es
    ret

AsmGetEs ENDP

;------------------------------------------------------------------------

AsmSetEs PROC

    mov     rax, rcx
    mov     es, rax 
    ret

AsmSetEs ENDP

;------------------------------------------------------------------------

AsmGetSs PROC

    mov     rax, ss
    ret

AsmGetSs ENDP

;------------------------------------------------------------------------

AsmSetSs PROC

    mov     rax, rcx
    mov     ss, rax 
    ret

AsmSetSs ENDP

;------------------------------------------------------------------------

AsmGetFs PROC

    mov     rax, fs
    ret

AsmGetFs ENDP

;------------------------------------------------------------------------

AsmSetFs PROC

    mov     rax, rcx
    mov     fs, rax 
    ret

AsmSetFs ENDP

;------------------------------------------------------------------------

AsmGetGs PROC

    mov     rax, gs
    ret

AsmGetGs ENDP

;------------------------------------------------------------------------

AsmGetLdtr PROC

    sldt    rax
    ret

AsmGetLdtr ENDP

;------------------------------------------------------------------------

AsmGetTr PROC

    str     rax
    ret

AsmGetTr ENDP

;------------------------------------------------------------------------

AsmGetIdtBase PROC

    LOCAL   idtr[10]:BYTE
    
    sidt    idtr
    mov     rax, QWORD PTR idtr[2]
    ret

AsmGetIdtBase ENDP

;------------------------------------------------------------------------

AsmGetGdtLimit PROC

    LOCAL    gdtr[10]:BYTE
    
    sgdt    gdtr
    mov     ax, WORD PTR gdtr[0]
    ret

AsmGetGdtLimit ENDP

;------------------------------------------------------------------------

AsmGetIdtLimit PROC

    LOCAL    idtr[10]:BYTE
    
    sidt    idtr
    mov     ax, WORD PTR idtr[0]
    ret

AsmGetIdtLimit ENDP

;------------------------------------------------------------------------

AsmGetAccessRights PROC
    lar     rax, rcx
    jz      no_error
    xor     rax, rax
no_error:
    ret
AsmGetAccessRights ENDP

END