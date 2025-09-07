/**
 * @file GlobalVariables.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Here we put global variables that are used more or less in all part of our hypervisor (not all of them)
 * @details Note : All the global variables are not here, just those that
 * will be used in all project. Special use global variables are located
 * in their corresponding headers
 *
 * @version 0.1
 * @date 2020-04-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				Global Variables				//
//////////////////////////////////////////////////

/**
 * @brief Different attributes and compatibility checks
 * of the current processor
 *
 */
COMPATIBILITY_CHECKS_STATUS g_CompatibilityCheck;

/**
 * @brief List of callbacks
 *
 */
VMM_CALLBACKS g_Callbacks;

/**
 * @brief Save the state and variables related to virtualization on each to logical core
 *
 */
VIRTUAL_MACHINE_STATE * g_GuestState;

/**
 * @brief Save the state of memory mapper
 *
 */
MEMORY_MAPPER_ADDRESSES * g_MemoryMapper;

/**
 * @brief Save the state and variables related to EPT
 *
 */
EPT_STATE * g_EptState;

/**
 * @brief List header of hidden hooks detour
 *
 */
LIST_ENTRY g_EptHook2sDetourListHead;

/**
 * @brief List header of hidden hooks detour
 *
 */
BOOLEAN g_IsEptHook2sDetourListInitialized;

/**
 * @brief Local APIC Base
 *
 */
VOID * g_ApicBase;

/**
 * @brief I/O APIC Base
 *
 */
VOID * g_IoApicBase;

/**
 * @brief check for broadcasting NMI mechanism support and its
 * initialization
 *
 */
BOOLEAN g_NmiBroadcastingInitialized;

/**
 * @brief NMI handler pointer for KeDeregisterNmiCallback
 *
 */
PVOID g_NmiHandlerForKeDeregisterNmiCallback;

/**
 * @brief Shows whether the debuggee is waiting for an
 * trap step or not
 *
 */
BOOLEAN g_IsUnsafeSyscallOrSysretHandling;

/**
 * @brief Bitmap of MSRs that cause #GP
 *
 */
UINT64 * g_MsrBitmapInvalidMsrs;

/**
 * @brief Enable interception of Cr3 for Mode-based Execution detection
 *
 */
BOOLEAN g_ModeBasedExecutionControlState;

/**
 * @brief State of syscall callback trap flags
 *
 */
SYSCALL_CALLBACK_TRAP_FLAG_STATE * g_SyscallCallbackTrapFlagState;

/**
 * @brief Shows whether the syscall callback is enabled or not
 *
 */
BOOLEAN g_SyscallCallbackStatus;

/**
 * @brief Target hook address for the system call handler
 *
 */
PVOID g_SystemCallHookAddress;

/**
 * @brief Shows whether the footprints (anti-debugging and
 * anti-hypervisor) should be checked or not
 *
 */
BOOLEAN g_CheckForFootprints;

//////////////////////////////////////////////////
//  	Global Variable (debugger-related)	    //
//////////////////////////////////////////////////

/**
 * @brief Showes whether the vmcall handler is
 * allowed to trigger an event or not
 *
 */
BOOLEAN g_TriggerEventForVmcalls;

/**
 * @brief Showes whether the cpuid handler is
 * allowed to trigger an event or not
 *
 */
BOOLEAN g_TriggerEventForCpuids;

BOOLEAN g_TriggerEventForXsetbvs;

//////////////////////////////////////////////////
//  	Global Variable (Execution Trap)	    //
//////////////////////////////////////////////////

/**
 * @brief Showes whether the execution trap handler is
 * allowed to trigger an event or not
 *
 */
BOOLEAN g_ExecTrapInitialized;

/**
 * @brief Showes whether the uninitialization of the exec trap is
 * started or not
 *
 */
BOOLEAN g_ExecTrapUnInitializationStarted;

/**
 * @brief State of the trap-flag
 *
 */
USER_KERNEL_EXECUTION_TRAP_STATE g_ExecTrapState;

/**
 * @brief Test value for intercepting instructions
 *
 */
BOOLEAN g_IsInterceptingInstructions;

//////////////////////////////////////////////////
//   Global Variable (page-fault injection)	    //
//////////////////////////////////////////////////

/**
 * @brief Shows whether the VMM is waiting to inject a page-fault
 * or not
 *
 */
BOOLEAN g_WaitingForInterruptWindowToInjectPageFault;

/**
 * @brief The (from) address for page-fault injection
 *
 */
UINT64 g_PageFaultInjectionAddressFrom;

/**
 * @brief The (to) address for page-fault injection
 *
 */
UINT64 g_PageFaultInjectionAddressTo;

/**
 * @brief The error code for page-fault injection
 *
 */
UINT32 g_PageFaultInjectionErrorCode;
