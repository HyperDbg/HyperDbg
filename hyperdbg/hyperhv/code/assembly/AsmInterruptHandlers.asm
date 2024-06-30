; This file was copied and modified from: https://github.com/jonomango/hv/blob/main/hv/interrupt-handlers.asm

EXTERN IdtEmulationhandleHostInterrupt:PROC

.code _text

;------------------------------------------------------------------------

; defined in IdtEmulation.h
trap_frame struct
  ; general-purpose registers
  $rax qword ?
  $rcx qword ?
  $rdx qword ?
  $rbx qword ?
  $rbp qword ?
  $rsi qword ?
  $rdi qword ?
  $r8  qword ?
  $r9  qword ?
  $r10 qword ?
  $r11 qword ?
  $r12 qword ?
  $r13 qword ?
  $r14 qword ?
  $r15 qword ?

  ; interrupt vector
  $vector qword ?

  ; _MACHINE_FRAME
  $error  qword ?
  $rip    qword ?
  $cs     qword ?
  $rflags qword ?
  $rsp    qword ?
  $ss     qword ?
trap_frame ends

;------------------------------------------------------------------------

; the generic interrupt handler that every stub will eventually jump to
GenericInterruptHandler proc
  ; allocate space for the trap_frame structure (minus the size of the
  ; _MACHINE_FRAME, error code, and interrupt vector)
  sub rsp, 78h

  ; general-purpose registers
  mov trap_frame.$rax[rsp], rax
  mov trap_frame.$rcx[rsp], rcx
  mov trap_frame.$rdx[rsp], rdx
  mov trap_frame.$rbx[rsp], rbx
  mov trap_frame.$rbp[rsp], rbp
  mov trap_frame.$rsi[rsp], rsi
  mov trap_frame.$rdi[rsp], rdi
  mov trap_frame.$r8[rsp],  r8
  mov trap_frame.$r9[rsp],  r9
  mov trap_frame.$r10[rsp], r10
  mov trap_frame.$r11[rsp], r11
  mov trap_frame.$r12[rsp], r12
  mov trap_frame.$r13[rsp], r13
  mov trap_frame.$r14[rsp], r14
  mov trap_frame.$r15[rsp], r15

  ; first argument is the trap frame
  mov rcx, rsp

  ; call IdtEmulationhandleHostInterrupt
  sub rsp, 20h
  call IdtEmulationhandleHostInterrupt
  add rsp, 20h

  ; general-purpose registers
  mov rax, trap_frame.$rax[rsp]
  mov rcx, trap_frame.$rcx[rsp]
  mov rdx, trap_frame.$rdx[rsp]
  mov rbx, trap_frame.$rbx[rsp]
  mov rbp, trap_frame.$rbp[rsp]
  mov rsi, trap_frame.$rsi[rsp]
  mov rdi, trap_frame.$rdi[rsp]
  mov r8,  trap_frame.$r8[rsp]
  mov r9,  trap_frame.$r9[rsp]
  mov r10, trap_frame.$r10[rsp]
  mov r11, trap_frame.$r11[rsp]
  mov r12, trap_frame.$r12[rsp]
  mov r13, trap_frame.$r13[rsp]
  mov r14, trap_frame.$r14[rsp]
  mov r15, trap_frame.$r15[rsp]

  ; free the trap_frame
  add rsp, 78h

  ; pop the interrupt vector
  add rsp, 8

  ; pop the error code
  add rsp, 8

  iretq

GenericInterruptHandler endp

;------------------------------------------------------------------------

; pushes error code to stack
DEFINE_ISR macro InterruptVector:req, ProcName:req
ProcName proc
  ; interrupt vector is stored right before the machine frame
  push InterruptVector

  jmp GenericInterruptHandler
ProcName endp
endm

; doesn't push error code to stack
DEFINE_ISR_NO_ERROR macro InterruptVector:req, ProcName:req
ProcName proc
  ; push a dummy error code onto the stack
  push 0

  ; interrupt vector is stored right before the machine frame
  push InterruptVector

  jmp GenericInterruptHandler
ProcName endp
endm

;------------------------------------------------------------------------

DEFINE_ISR_NO_ERROR 0,  InterruptHandler0
DEFINE_ISR_NO_ERROR 1,  InterruptHandler1
DEFINE_ISR_NO_ERROR 2,  InterruptHandler2
DEFINE_ISR_NO_ERROR 3,  InterruptHandler3
DEFINE_ISR_NO_ERROR 4,  InterruptHandler4
DEFINE_ISR_NO_ERROR 5,  InterruptHandler5
DEFINE_ISR_NO_ERROR 6,  InterruptHandler6
DEFINE_ISR_NO_ERROR 7,  InterruptHandler7
DEFINE_ISR          8,  InterruptHandler8
DEFINE_ISR          10, InterruptHandler10
DEFINE_ISR          11, InterruptHandler11
DEFINE_ISR          12, InterruptHandler12
DEFINE_ISR          13, InterruptHandler13
DEFINE_ISR          14, InterruptHandler14
DEFINE_ISR_NO_ERROR 16, InterruptHandler16
DEFINE_ISR          17, InterruptHandler17
DEFINE_ISR_NO_ERROR 18, InterruptHandler18
DEFINE_ISR_NO_ERROR 19, InterruptHandler19
DEFINE_ISR_NO_ERROR 20, InterruptHandler20
DEFINE_ISR          30, InterruptHandler30

;------------------------------------------------------------------------

end