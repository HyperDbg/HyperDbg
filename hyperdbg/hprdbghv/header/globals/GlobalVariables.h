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
 * @brief Save the state and variables related to virtualization on each to logical core
 *
 */
VIRTUAL_MACHINE_STATE* g_GuestState;

/**
 * @brief Save the state of memory mapper
 *
 */
MEMORY_MAPPER_ADDRESSES* g_MemoryMapper;

/**
 * @brief Save the state and variables related to EPT
 *
 */
EPT_STATE* g_EptState;

/**
 * @brief Support for execute-only pages (indicating that data accesses are
 *  not allowed while instruction fetches are allowed)
 *
 */
BOOLEAN g_ExecuteOnlySupport;

/**
 * @brief holds the measurements from the user-mode and kernel-mode
 *
 */
TRANSPARENCY_MEASUREMENTS* g_TransparentModeMeasurements;

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
 * @brief Shows whether the debugger transparent mode
 * is enabled (true) or not (false)
 *
 */
BOOLEAN g_TransparentMode;

/**
 * @brief X2APIC or XAPIC routine
 *
 */
BOOLEAN g_IsX2Apic;

/**
 * @brief APIC Base
 *
 */
VOID* g_ApicBase;

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
 * @brief Shows whether the debuggee is waiting for an
 * trap step or not
 *
 */
BOOLEAN g_IsUnsafeSyscallOrSysretHandling;

/**
 * @brief Bitmap of MSRs that cause #GP
 *
 */
UINT64* g_MsrBitmapInvalidMsrs;

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
