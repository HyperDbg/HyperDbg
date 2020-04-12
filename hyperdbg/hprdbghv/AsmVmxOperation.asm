PUBLIC AsmEnableVmxOperation
PUBLIC AsmVmxVmcall
PUBLIC AsmHypervVmcall


.code _text

;------------------------------------------------------------------------

AsmEnableVmxOperation PROC PUBLIC

    xor rax,rax			; Clear the RAX
    mov rax,cr4
    or rax,02000h		; Set the 14th bit
    mov cr4,rax

    ret

AsmEnableVmxOperation ENDP


;------------------------------------------------------------------------

AsmVmxVmcall PROC
    ; We change r10 to HVFS Hex ASCII and r11 to VMCALL Hex ASCII and r12 to NOHYPERV Hex ASCII so we can make sure that the calling Vmcall comes
    ; from our hypervisor and we're resposible for managing it, otherwise it has to be managed by Hyper-V
    pushfq
    push    r10
    push    r11
    push    r12
    mov     r10, 48564653H          ; [HVFS]
    mov     r11, 564d43414c4cH      ; [VMCALL]
    mov     r12, 4e4f485950455256H   ; [NOHYPERV]
    vmcall                          ; VmxVmcallHandler(UINT64 VmcallNumber, UINT64 OptionalParam1, UINT64 OptionalParam2, UINT64 OptionalParam3)
    pop     r12
    pop     r11
    pop     r10
    popfq
    ret                             ; Return type is NTSTATUS and it's on RAX from the previous function, no need to change anything

AsmVmxVmcall ENDP


;------------------------------------------------------------------------

AsmHypervVmcall PROC
    vmcall                       ; __fastcall Vmcall(rcx = HypercallInputValue, rdx = InputParamGPA, r8 = OutputParamGPA)
    ret

AsmHypervVmcall ENDP

;------------------------------------------------------------------------


END