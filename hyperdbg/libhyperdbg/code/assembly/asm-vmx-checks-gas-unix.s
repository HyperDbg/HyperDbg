/* ------------------------------------------------------------------------
 * GAS (AT&T syntax) port of asm-vmx-checks-masm-windows.asm
 *
 * AsmVmxSupportDetection: returns 1 in rax if the CPU reports VMX support
 * (CPUID.1:ECX[5]), 0 otherwise.
 * ------------------------------------------------------------------------ */

    .text
    .globl AsmVmxSupportDetection
    .type  AsmVmxSupportDetection, @function

/* ------------------------------------------------------------------------ */

AsmVmxSupportDetection:
    push    %rbx
    push    %rcx
    push    %rdx

    xor     %eax, %eax
    inc     %eax
    cpuid
    xor     %rax, %rax
    bt      $0x05, %ecx
    jc      VMXSupport

VMXNotSupport:
    jmp     RetInst

VMXSupport:
    mov     $0x01, %rax

RetInst:
    pop     %rdx
    pop     %rcx
    pop     %rbx

    ret

    .size AsmVmxSupportDetection, .-AsmVmxSupportDetection

/* ------------------------------------------------------------------------ */

/* Mark the stack as non-executable (no executable-stack requirement). */
    .section .note.GNU-stack,"",@progbits
