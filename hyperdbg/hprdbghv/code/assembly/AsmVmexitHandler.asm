PUBLIC AsmVmexitHandler

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

    pushfq

    ; ------------ Save XMM Registers ------------
    ;
    ;   ;;;;;;;;;;;; 16 Byte * 16 Byte = 256 + 4  = 260 (0x106 == 0x110 but let's align it to have better performance) ;;;;;;;;;;;;
    ;   sub     rsp, 0110h
    ;
    ;   movaps  xmmword ptr [rsp+000h], xmm0    ; each xmm register 128 bit (16 Byte)
    ;   movaps  xmmword ptr [rsp+010h], xmm1
    ;   movaps  xmmword ptr [rsp+020h], xmm2
    ;   movaps  xmmword ptr [rsp+030h], xmm3
    ;   movaps  xmmword ptr [rsp+040h], xmm4
    ;   movaps  xmmword ptr [rsp+050h], xmm5
    ;   movaps  xmmword ptr [rsp+060h], xmm6 
    ;   movaps  xmmword ptr [rsp+070h], xmm7
    ;   movaps  xmmword ptr [rsp+080h], xmm8
    ;   movaps  xmmword ptr [rsp+090h], xmm9
    ;   movaps  xmmword ptr [rsp+0a0h], xmm10
    ;   movaps  xmmword ptr [rsp+0b0h], xmm11
    ;   movaps  xmmword ptr [rsp+0c0h], xmm12
    ;   movaps  xmmword ptr [rsp+0d0h], xmm13
    ;   movaps  xmmword ptr [rsp+0e0h], xmm14
    ;   movaps  xmmword ptr [rsp+0f0h], xmm15 
    ;   stmxcsr dword ptr [rsp+0100h]           ; MxCsr is 4 Byte
    ;
    ;---------------------------------------------

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
    
    mov rcx, rsp		; Fast call argument to PGUEST_REGS
    sub	rsp, 020h		; Free some space for Shadow Section
    call	VmxVmexitHandler
    add	rsp, 020h		; Restore the state
    
    cmp	al, 1	; Check whether we have to turn off VMX or Not (the result is in RAX)
    je		AsmVmxoffHandler
    
RestoreState:
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

    ; ------------ Restore XMM Registers ------------
    ;
    ;   movaps xmm0, xmmword ptr [rsp+000h]
    ;   movaps xmm1, xmmword ptr [rsp+010h]
    ;   movaps xmm2, xmmword ptr [rsp+020h]
    ;   movaps xmm3, xmmword ptr [rsp+030h]
    ;   movaps xmm4, xmmword ptr [rsp+040h]
    ;   movaps xmm5, xmmword ptr [rsp+050h]
    ;   movaps xmm6, xmmword ptr [rsp+060h]
    ;   movaps xmm7, xmmword ptr [rsp+070h]
    ;   movaps xmm8, xmmword ptr [rsp+080h]
    ;   movaps xmm9, xmmword ptr [rsp+090h]
    ;   movaps xmm10, xmmword ptr [rsp+0a0h]
    ;   movaps xmm11, xmmword ptr [rsp+0b0h]
    ;   movaps xmm12, xmmword ptr [rsp+0c0h]
    ;   movaps xmm13, xmmword ptr [rsp+0d0h]
    ;   movaps xmm14, xmmword ptr [rsp+0e0h]
    ;   movaps xmm15, xmmword ptr [rsp+0f0h]
    ;
    ;   ldmxcsr dword ptr [rsp+0100h]          
    ;   
    ;   add     rsp, 0110h
    ; ----------------------------------------------

    popfq

    sub rsp, 0100h      ; to avoid error in future functions
    jmp VmxVmresume
    
AsmVmexitHandler ENDP

;------------------------------------------------------------------------

AsmVmxoffHandler PROC
    
    sub rsp, 020h ; shadow space
    call VmxReturnStackPointerForVmxoff
    add rsp, 020h ; remove for shadow space
    
    mov [rsp+88h], rax  ; now, rax contains rsp
    
    sub rsp, 020h      ; shadow space
    call VmxReturnInstructionPointerForVmxoff
    add rsp, 020h      ; remove for shadow space
    
    mov rdx, rsp       ; save current rsp
    
    mov rbx, [rsp+88h] ; read rsp again
    
    mov rsp, rbx
    
    push rax            ; push the return address as we changed the stack, we push
                  		; it to the new stack
    
    mov rsp, rdx        ; restore previous rsp
                    
    sub rbx,08h         ; we push sth, so we have to add (sub) +8 from previous stack
                   		; also rbx already contains the rsp
    mov [rsp+88h], rbx  ; move the new pointer to the current stack
    
RestoreState:
    pop rax
    pop rcx
    pop rdx
    pop rbx
    pop rbp		         ; rsp
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
    ;
    ;    movaps xmm0, xmmword ptr [rsp+000h]
    ;    movaps xmm1, xmmword ptr [rsp+010h]
    ;    movaps xmm2, xmmword ptr [rsp+020h]
    ;    movaps xmm3, xmmword ptr [rsp+030h]
    ;    movaps xmm4, xmmword ptr [rsp+040h]
    ;    movaps xmm5, xmmword ptr [rsp+050h]
    ;    movaps xmm6, xmmword ptr [rsp+060h]
    ;    movaps xmm7, xmmword ptr [rsp+070h]
    ;    movaps xmm8, xmmword ptr [rsp+080h]
    ;    movaps xmm9, xmmword ptr [rsp+090h]
    ;    movaps xmm10, xmmword ptr [rsp+0a0h]
    ;    movaps xmm11, xmmword ptr [rsp+0b0h]
    ;    movaps xmm12, xmmword ptr [rsp+0c0h]
    ;    movaps xmm13, xmmword ptr [rsp+0d0h]
    ;    movaps xmm14, xmmword ptr [rsp+0e0h]
    ;    movaps xmm15, xmmword ptr [rsp+0f0h]
    ;
    ;    ldmxcsr dword ptr [rsp+0100h]          
    ;    
    ;    add     rsp, 0110h
    ; ----------------------------------------------

    popfq
    pop		rsp     ; restore rsp

    ret             ; jump back to where we called Vmcall

AsmVmxoffHandler ENDP

;------------------------------------------------------------------------

END
