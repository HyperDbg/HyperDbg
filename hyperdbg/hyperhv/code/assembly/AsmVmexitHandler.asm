PUBLIC AsmVmexitHandler
PUBLIC AsmVmxoffRestoreXmmRegs

EXTERN VmxVmexitHandler:PROC
EXTERN VmxVmresume:PROC
EXTERN VmxReturnStackPointerForVmxoff:PROC
EXTERN VmxReturnInstructionPointerForVmxoff:PROC

.code _text

;------------------------------------------------------------------------

AsmVmexitHandler PROC
    
    push 0  ; we might be in an unaligned stack state, so the memory before stack might cause 
            ; irql less or equal as it doesn't exist, so we just put some extra space avoid
            ; these kind of errors

    ; --------------- Save RFLAGS ----------------

    pushfq  ; Save the flags register (RFLAGS)

    ; ------------ Save XMM Registers ------------

    ; 16 Byte * 16 Byte = 256 + 4  = 260 (0x104 == 0x110 but let's align it to have better performance)
    
    sub     rsp, 0110h

    movaps  xmmword ptr [rsp+000h], xmm0    ; each xmm register 128 bit (16 Byte)
    movaps  xmmword ptr [rsp+010h], xmm1
    movaps  xmmword ptr [rsp+020h], xmm2
    movaps  xmmword ptr [rsp+030h], xmm3
    movaps  xmmword ptr [rsp+040h], xmm4
    movaps  xmmword ptr [rsp+050h], xmm5

    ;
    ; As per Microsoft ABI documentation, the following registers are nonvolatile
    ; So, MSVC compiler will save them on the stack if they are used in the function
    ; Thus, for the sake of performance, we comment them out
    ;
    ; movaps  xmmword ptr [rsp+060h], xmm6 
    ; movaps  xmmword ptr [rsp+070h], xmm7
    ; movaps  xmmword ptr [rsp+080h], xmm8
    ; movaps  xmmword ptr [rsp+090h], xmm9
    ; movaps  xmmword ptr [rsp+0a0h], xmm10
    ; movaps  xmmword ptr [rsp+0b0h], xmm11
    ; movaps  xmmword ptr [rsp+0c0h], xmm12
    ; movaps  xmmword ptr [rsp+0d0h], xmm13
    ; movaps  xmmword ptr [rsp+0e0h], xmm14
    ; movaps  xmmword ptr [rsp+0f0h], xmm15 

    stmxcsr dword ptr [rsp+0100h]           ; MxCsr is 4 Byte

    ; ------ Save General-purpose Registers ------

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

    ; ----------- Call VM-exit Handler -----------

    mov rcx, rsp		; Fast call argument to PGUEST_REGS

    sub	rsp, 020h		; Free some space for Shadow Section
    call	VmxVmexitHandler
    add	rsp, 020h		; Restore the state
    
    cmp	al, 1	        ; Check whether we have to turn off VMX or Not (the result is in RAX)

    je		AsmVmxoffHandler

    ; ----------- Restore XMM Registers ----------

RestoreState:

    pop rax
    pop rcx
    pop rdx
    pop rbx
    pop rbp		        ; rsp
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

    ; ------------ Restore XMM Registers ------------

    movaps xmm0, xmmword ptr [rsp+000h]
    movaps xmm1, xmmword ptr [rsp+010h]
    movaps xmm2, xmmword ptr [rsp+020h]
    movaps xmm3, xmmword ptr [rsp+030h]
    movaps xmm4, xmmword ptr [rsp+040h]
    movaps xmm5, xmmword ptr [rsp+050h]

    ;
    ; As per Microsoft ABI documentation, the following registers are nonvolatile
    ; So, MSVC compiler will save them on the stack if they are used in the function
    ; Thus, for the sake of performance, we comment them out
    ;
    ; movaps xmm6, xmmword ptr [rsp+060h]
    ; movaps xmm7, xmmword ptr [rsp+070h]
    ; movaps xmm8, xmmword ptr [rsp+080h]
    ; movaps xmm9, xmmword ptr [rsp+090h]
    ; movaps xmm10, xmmword ptr [rsp+0a0h]
    ; movaps xmm11, xmmword ptr [rsp+0b0h]
    ; movaps xmm12, xmmword ptr [rsp+0c0h]
    ; movaps xmm13, xmmword ptr [rsp+0d0h]
    ; movaps xmm14, xmmword ptr [rsp+0e0h]
    ; movaps xmm15, xmmword ptr [rsp+0f0h]
    
    ldmxcsr dword ptr [rsp+0100h]          
    
    add     rsp, 0110h

    ; --------------- Restore RFLAGS ---------------

    popfq               ; Restore the flags register (RFLAGS)

    ; ----------------------------------------------

    jmp VmxVmresume
    
AsmVmexitHandler ENDP

;------------------------------------------------------------------------

AsmVmxoffRestoreXmmRegs PROC

    ; ------------ Restore XMM Registers ------------

    movaps xmm0, xmmword ptr [rcx+000h]
    movaps xmm1, xmmword ptr [rcx+010h]
    movaps xmm2, xmmword ptr [rcx+020h]
    movaps xmm3, xmmword ptr [rcx+030h]
    movaps xmm4, xmmword ptr [rcx+040h]
    movaps xmm5, xmmword ptr [rcx+050h]

    ;
    ; As per Microsoft ABI documentation, the following registers are nonvolatile
    ; So, MSVC compiler will save them on the stack if they are used in the function
    ; Thus, for the sake of performance, we comment them out
    ;
    ; movaps xmm6, xmmword ptr [rcx+060h]
    ; movaps xmm7, xmmword ptr [rcx+070h]
    ; movaps xmm8, xmmword ptr [rcx+080h]
    ; movaps xmm9, xmmword ptr [rcx+090h]
    ; movaps xmm10, xmmword ptr [rcx+0a0h]
    ; movaps xmm11, xmmword ptr [rcx+0b0h]
    ; movaps xmm12, xmmword ptr [rcx+0c0h]
    ; movaps xmm13, xmmword ptr [rcx+0d0h]
    ; movaps xmm14, xmmword ptr [rcx+0e0h]
    ; movaps xmm15, xmmword ptr [rcx+0f0h]

    ldmxcsr dword ptr [rcx+0100h]

    ret

AsmVmxoffRestoreXmmRegs ENDP

;------------------------------------------------------------------------

AsmVmxoffHandler PROC

    ; ------ Restore General-purpose Registers ------

RestoreState:

    pop rax
    pop rcx
    pop rdx
    pop rbx
    pop rbp		        ; rsp
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

    ; ------------- Pass over XMM Regs -------------

    ; XMM registers are restored by AsmVmxoffRestoreXmmRegs
    ; Size of XMM registers are 110 bytes but we also remove the RFLAGS register (+8) 
    ; so we have 118 bytes to remove from the stack
    add     rsp, 0118h

    ; ------------ Get Stack Pointer ---------------

    sub rsp, 020h       ; shadow space
    call VmxReturnStackPointerForVmxoff
    add rsp, 020h       ; remove for shadow space

    push rax            ; save the current rsp, we will restore it later


    ; ------------ Get Instruction Pointer ---------

    sub rsp, 020h       ; shadow space
    call VmxReturnInstructionPointerForVmxoff
    add rsp, 020h       ; remove for shadow space

    pop		rsp         ; restore rsp
    push rax            ; push the instruction pointer

    ; ----------------------------------------------

    ; There might be some registers that are modified by above CALLs, 
    ; but here we do not care about them since we are also in a function, 
    ; so the caller do not expect us to preserve these volatile registers

    xor rax, rax        ; clear RAX to indicate VMXOFF was successful (VMCALL Status)

    ret                 ; jump back to where we called Vmcall

AsmVmxoffHandler ENDP

;------------------------------------------------------------------------

END
