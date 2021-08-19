PUBLIC AsmVmxSupportDetection

.code _text

;------------------------------------------------------------------------
; Note : Right-click the .asm file, Properties, change Item Type to
; "Microsoft Macro Assembler" if it didn't compile
;------------------------------------------------------------------------

;------------------------------------------------------------------------

AsmVmxSupportDetection PROC

    xor eax, eax
    inc    eax
    cpuid
    xor rax, rax
    bt     ecx, 05h
    jc     VMXSupport
    
VMXNotSupport:
    jmp     RetInst
    
VMXSupport:
    mov    rax, 01h
    
RetInst:
    ret

AsmVmxSupportDetection ENDP

;------------------------------------------------------------------------

END                     