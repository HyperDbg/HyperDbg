/**
 * @file Datatypes.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief HyperDbg's SDK data type definitions
 * @details This file contains definitions of structures, enums, etc.
 * used in HyperDbg
 * @version 0.2
 * @date 2022-06-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//            Callback Definitions              //
//////////////////////////////////////////////////

/**
 * @brief Callback type that can be used to be used
 * as a custom ShowMessages function
 *
 */
typedef int (*Callback)(const char * Text);

/**
 * @brief Integer gp registers (this structure is defined in
 * two places, make sure to change it in two places)
 *
 */
#ifndef GUEST_REGS_DEFINED
#    define GUEST_REGS_DEFINED

//
// DO NOT FUCKING TOUCH THIS STRUCTURE WITHOUT COORDINATION WITH SINA
//
typedef struct GUEST_REGS
{
    //
    // DO NOT FUCKING TOUCH THIS STRUCTURE WITHOUT COORDINATION WITH SINA
    //

    UINT64 rax; // 0x00
    UINT64 rcx; // 0x08
    UINT64 rdx; // 0x10
    UINT64 rbx; // 0x18
    UINT64 rsp; // 0x20
    UINT64 rbp; // 0x28
    UINT64 rsi; // 0x30
    UINT64 rdi; // 0x38
    UINT64 r8;  // 0x40
    UINT64 r9;  // 0x48
    UINT64 r10; // 0x50
    UINT64 r11; // 0x58
    UINT64 r12; // 0x60
    UINT64 r13; // 0x68
    UINT64 r14; // 0x70
    UINT64 r15; // 0x78

    //
    // DO NOT FUCKING TOUCH THIS STRUCTURE WITHOUT COORDINATION WITH SINA
    //

} GUEST_REGS, *PGUEST_REGS;
#endif

/**
 * @brief struct for extra registers
 *
 */
typedef struct GUEST_EXTRA_REGISTERS
{
    UINT16 CS;
    UINT16 DS;
    UINT16 FS;
    UINT16 GS;
    UINT16 ES;
    UINT16 SS;
    UINT64 RFLAGS;
    UINT64 RIP;
} GUEST_EXTRA_REGISTERS, *PGUEST_EXTRA_REGISTERS;

//////////////////////////////////////////////////
//               Event Details                  //
//////////////////////////////////////////////////

/**
 * @brief enum to show type of all HyperDbg events
 *
 */
typedef enum _DEBUGGER_EVENT_TYPE_ENUM
{

    HIDDEN_HOOK_READ_AND_WRITE,
    HIDDEN_HOOK_READ,
    HIDDEN_HOOK_WRITE,

    HIDDEN_HOOK_EXEC_DETOURS,
    HIDDEN_HOOK_EXEC_CC,

    SYSCALL_HOOK_EFER_SYSCALL,
    SYSCALL_HOOK_EFER_SYSRET,

    CPUID_INSTRUCTION_EXECUTION,

    RDMSR_INSTRUCTION_EXECUTION,
    WRMSR_INSTRUCTION_EXECUTION,

    IN_INSTRUCTION_EXECUTION,
    OUT_INSTRUCTION_EXECUTION,

    EXCEPTION_OCCURRED,
    EXTERNAL_INTERRUPT_OCCURRED,

    DEBUG_REGISTERS_ACCESSED,

    TSC_INSTRUCTION_EXECUTION,
    PMC_INSTRUCTION_EXECUTION,

    VMCALL_INSTRUCTION_EXECUTION,

} DEBUGGER_EVENT_TYPE_ENUM;

/**
 * @brief Type of Actions
 *
 */
typedef enum _DEBUGGER_EVENT_ACTION_TYPE_ENUM
{
    BREAK_TO_DEBUGGER,
    RUN_SCRIPT,
    RUN_CUSTOM_CODE

} DEBUGGER_EVENT_ACTION_TYPE_ENUM;

/**
 * @brief Type of handling !syscall or !sysret
 *
 */
typedef enum _DEBUGGER_EVENT_SYSCALL_SYSRET_TYPE
{
    DEBUGGER_EVENT_SYSCALL_SYSRET_SAFE_ACCESS_MEMORY = 0,
    DEBUGGER_EVENT_SYSCALL_SYSRET_HANDLE_ALL_UD      = 1,

} DEBUGGER_EVENT_SYSCALL_SYSRET_TYPE;
