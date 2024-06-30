PUBLIC AsmStiInstruction
PUBLIC AsmCliInstruction
PUBLIC AsmGetRflags
PUBLIC AsmReloadGdtr
PUBLIC AsmReloadIdtr

.code _text

;------------------------------------------------------------------------

AsmStiInstruction PROC PUBLIC

    sti
    ret

AsmStiInstruction ENDP 

;------------------------------------------------------------------------

AsmCliInstruction PROC PUBLIC

    cli
    ret

AsmCliInstruction ENDP 

;------------------------------------------------------------------------

AsmGetRflags PROC
    
    pushfq
    pop		rax
    ret
    
AsmGetRflags ENDP

;------------------------------------------------------------------------

; AsmReloadGdtr (PVOID GdtBase (rcx), UINT32 GdtLimit (rdx) );

AsmReloadGdtr PROC

    push	rcx
    shl		rdx, 48
    push	rdx
    lgdt	fword ptr [rsp+6]	; do not try to modify stack selector with this ;)
    pop		rax
    pop		rax
    ret
    
AsmReloadGdtr ENDP

;------------------------------------------------------------------------

; AsmReloadIdtr (PVOID IdtBase (rcx), UINT32 IdtLimit (rdx) );

AsmReloadIdtr PROC
    
    push	rcx
    shl		rdx, 48
    push	rdx
    lidt	fword ptr [rsp+6]
    pop		rax
    pop		rax
    ret
    
AsmReloadIdtr ENDP

;------------------------------------------------------------------------

END                     