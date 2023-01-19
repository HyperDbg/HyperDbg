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
 * @brief List of callbacks
 *
 */
VMM_CALLBACKS g_Callbacks;

/**
 * @brief The value of last error
 *
 */
UINT32 g_LastError;

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
 * @brief Holder of script engines global variables
 *
 */
UINT64 * g_ScriptGlobalVariables;

/**
 * @brief Support for execute-only pages (indicating that data accesses are
 *  not allowed while instruction fetches are allowed)
 *
 */
BOOLEAN g_ExecuteOnlySupport;

/**
 * @brief Determines whether the clients are allowed to send IOCTL to the drive or not
 *
 */
BOOLEAN g_AllowIOCTLFromUsermode;

/**
 * @brief holds the measurements from the user-mode and kernel-mode
 *
 */
TRANSPARENCY_MEASUREMENTS * g_TransparentModeMeasurements;

/**
 * @brief Determines whether the debugger events should be active or not
 *
 */
BOOLEAN g_EnableDebuggerEvents;

/**
 * @brief List header of hidden hooks detour
 *
 */
LIST_ENTRY g_EptHook2sDetourListHead;

/**
 * @brief List header of breakpoints for debugger-mode
 *
 */
LIST_ENTRY g_BreakpointsListHead;

/**
 * @brief Seed for setting id of breakpoints
 *
 */
UINT64 g_MaximumBreakpointId;

/**
 * @brief Shows whether the debugger transparent mode
 * is enabled (true) or not (false)
 *
 */
BOOLEAN g_TransparentMode;

/**
 * @brief shows whether the kernel debugger is enabled or disabled
 *
 */
BOOLEAN g_KernelDebuggerState;

/**
 * @brief shows whether the user debugger is enabled or disabled
 *
 */
BOOLEAN g_UserDebuggerState;

/**
 * @brief X2APIC or XAPIC routine
 *
 */
BOOLEAN g_IsX2Apic;

/**
 * @brief APIC Base
 *
 */
VOID * g_ApicBase;

/**
 * @brief Reason that the debuggee is halted
 *
 */
DEBUGGEE_PAUSING_REASON g_DebuggeeHaltReason;

/**
 * @brief Optional context as the debuggee is halted
 *
 */
PVOID g_DebuggeeHaltContext;

/**
 * @brief Optional tag as the debuggee is halted
 *
 */
UINT64 g_DebuggeeHaltTag;

/**
 * @brief NMI handler pointer for KeDeregisterNmiCallback
 *
 */
PVOID g_NmiHandlerForKeDeregisterNmiCallback;

/**
 * @brief check for broadcasting NMI mechanism support and its
 * initialization
 *
 */
BOOLEAN g_NmiBroadcastingInitialized;

/**
 * @brief check for RTM support
 *
 */
BOOLEAN g_RtmSupport;

/**
 * @brief Virtual address width for x86 processors
 *
 */
UINT32 g_VirtualAddressWidth;

/**
 * @brief Target function for kernel tests
 *
 */
PVOID g_KernelTestTargetFunction;

/**
 * @brief Tag1 for kernel tests
 *
 */
UINT64 g_KernelTestTag1;

/**
 * @brief Tag2 for kernel tests
 *
 */
UINT64 g_KernelTestTag2;

/**
 * @brief Temp registers for kernel tests
 *
 */
UINT64 g_KernelTestR15;
UINT64 g_KernelTestR14;
UINT64 g_KernelTestR13;
UINT64 g_KernelTestR12;

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
 * @brief Seed for tokens of unique details buffer for threads
 *
 */
UINT64 g_SeedOfUserDebuggingDetails;

/**
 * @brief Whether the thread attaching mechanism is waiting for #DB or not
 *
 */
BOOLEAN g_IsWaitingForUserModeModuleEntrypointToBeCalled;

/**
 * @brief Whether the thread attaching mechanism is waiting for a page-fault
 * finish or not
 *
 */
BOOLEAN g_IsWaitingForReturnAndRunFromPageFault;

/**
 * @brief List header of thread debugging details
 *
 */
LIST_ENTRY g_ProcessDebuggingDetailsListHead;

/**
 * @brief Whether the page-fault and cr3 vm-exits in vmx-root should check
 * the #PFs or the PML4.Supervisor with user debugger or not
 *
 */
BOOLEAN g_CheckPageFaultsAndMov2Cr3VmexitsWithUserDebugger;

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
