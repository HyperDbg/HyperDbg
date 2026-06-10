/**
 * @file HyperTrace.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief HyperDbg's SDK for hypertrace project
 * @details This file contains definitions of HyperTrace routines
 * @version 0.14
 * @date 2025-06-07
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//			     Callback Types                 //
//////////////////////////////////////////////////

/**
 * @brief A function from the message tracer that send the inputs to the
 * queue of the messages
 *
 */
typedef BOOLEAN (*LOG_CALLBACK_PREPARE_AND_SEND_MESSAGE_TO_QUEUE)(UINT32       OperationCode,
                                                                  BOOLEAN      IsImmediateMessage,
                                                                  BOOLEAN      ShowCurrentSystemTime,
                                                                  BOOLEAN      Priority,
                                                                  const CHAR * Fmt,
                                                                  va_list      ArgList);

/**
 * @brief A function that sends the messages to message tracer buffers
 *
 */
typedef BOOLEAN (*LOG_CALLBACK_SEND_MESSAGE_TO_QUEUE)(UINT32 OperationCode, BOOLEAN IsImmediateMessage, CHAR * LogMessage, UINT32 BufferLen, BOOLEAN Priority);

/**
 * @brief A function that sends the messages to message tracer buffers
 *
 */
typedef BOOLEAN (*LOG_CALLBACK_SEND_BUFFER)(_In_ UINT32                          OperationCode,
                                            _In_reads_bytes_(BufferLength) PVOID Buffer,
                                            _In_ UINT32                          BufferLength,
                                            _In_ BOOLEAN                         Priority);

/**
 * @brief A function that checks whether the priority or regular buffer is full or not
 *
 */
typedef BOOLEAN (*LOG_CALLBACK_CHECK_IF_BUFFER_IS_FULL)(BOOLEAN Priority);

/**
 * @brief A function that gets the guest state of IA32_DEBUGCTL
 */
typedef UINT64 (*VM_FUNC_GET_DEBUGCTL)();

/**
 * @brief A function that gets the guest state of IA32_DEBUGCTL on the target core using VMCALL
 */
typedef UINT64 (*VM_FUNC_GET_DEBUGCTL_VMCALL_ON_TARGET_CORE)();

/**
 * @brief A function that gets the guest state of IA32_LBR_CTL
 */
typedef UINT64 (*VM_FUNC_GET_GUEST_IA32_LBR_CTL)();

/**
 * @brief A function that gets the guest state of IA32_LBR_CTL on the target core using VMCALL
 */
typedef UINT64 (*VM_FUNC_GET_GUEST_IA32_LBR_CTL_VMCALL_ON_TARGET_CORE)();

/**
 * @brief A function that gets the guest state of IA32_DEBUGCTL
 *
 */
typedef VOID (*VM_FUNC_SET_DEBUGCTL)(UINT64 Value);

/**
 * @brief A function that gets the guest state of IA32_DEBUGCTL on the target core using VMCALL
 *
 */
typedef VOID (*VM_FUNC_SET_DEBUGCTL_VMCALL_ON_TARGET_CORE)(UINT64 Value);

/**
 * @brief A function that sets guest IA32_LBR_CTL
 *
 */
typedef VOID (*VM_FUNC_SET_GUEST_IA32_LBR_CTL)(UINT64 Value);

/**
 * @brief A function that sets guest IA32_LBR_CTL on the target core using VMCALL
 *
 */
typedef VOID (*VM_FUNC_SET_GUEST_IA32_LBR_CTL_VMCALL_ON_TARGET_CORE)(UINT64 Value);

/**
 * @brief A function that set MSR_LEGACY_LBR_SELECT
 *
 */
typedef VOID (*VM_FUNC_SET_LBR_SELECT)(UINT64 FilterOptions);

/**
 * @brief A function that set MSR_LEGACY_LBR_SELECT on the target core using VMCALL
 *
 */
typedef VOID (*VM_FUNC_SET_LBR_SELECT_VMCALL_ON_TARGET_CORE)(UINT64 FilterOptions);

/**
 * @brief A function that checks whether IA32_DEBUGCTL can be used in load and save of exit and entry controls
 *
 */
typedef BOOLEAN (*VM_FUNC_CHECK_CPU_SUPPORT_FOR_SAVE_AND_LOAD_DEBUG_CONTROLS)();

/**
 * @brief A function that checks whether guest IA32_LBR_CTL can be used in load and clear of guest IA32_LBR_CTL controls
 *
 */
typedef BOOLEAN (*VM_FUNC_CHECK_CPU_SUPPORT_FOR_LOAD_AND_CLEAR_GUEST_IA32_LBR_CTL_CONTROLS)();

/**
 * @brief A function that sets load debug controls on VM-entry controls
 *
 */
typedef VOID (*VM_FUNC_SET_LOAD_DEBUG_CONTROLS)(UINT32 CoreId, BOOLEAN Set);

/**
 * @brief A function that sets load debug controls on VM-entry controls on the target core from VMCS using VMCALL
 *
 */
typedef VOID (*VM_FUNC_SET_LOAD_DEBUG_CONTROLS_VMCALL_ON_TARGET_CORE)(BOOLEAN Set);

/**
 * @brief A function that sets load guest IA32_LBR_CTL on VM-entry controls
 *
 */
typedef VOID (*VM_FUNC_SET_LOAD_GUEST_IA32_LBR_CTL)(UINT32 CoreId, BOOLEAN Set);

/**
 * @brief A function that sets load guest IA32_LBR_CTL on VM-entry controls on the target core from VMCS using VMCALL
 *
 */
typedef VOID (*VM_FUNC_SET_LOAD_GUEST_IA32_LBR_CTL_VMCALL_ON_TARGET_CORE)(BOOLEAN Set);

/**
 * @brief A function that sets save debug controls on VM-exit controls
 *
 */
typedef VOID (*VM_FUNC_SET_SAVE_DEBUG_CONTROLS)(UINT32 CoreId, BOOLEAN Set);

/**
 * @brief A function that sets save debug controls on VM-exit controls on the target core from VMCS using VMCALL
 *
 */
typedef VOID (*VM_FUNC_SET_SAVE_DEBUG_CONTROLS_VMCALL_ON_TARGET_CORE)(BOOLEAN Set);

/**
 * @brief A function that sets clear guest IA32_LBR_CTL on VM-exit controls
 *
 */
typedef VOID (*VM_FUNC_SET_CLEAR_GUEST_IA32_LBR_CTL)(UINT32 CoreId, BOOLEAN Set);

/**
 * @brief A function that sets clear guest IA32_LBR_CTL on VM-exit controls on the target core from VMCS using VMCALL
 *
 */
typedef VOID (*VM_FUNC_SET_CLEAR_GUEST_IA32_LBR_CTL_VMCALL_ON_TARGET_CORE)(BOOLEAN Set);

/**
 * @brief A function that checks whether the current execution mode is VMX-root mode or not
 *
 */
typedef BOOLEAN (*VM_FUNC_VMX_GET_CURRENT_EXECUTION_MODE)();

/**
 * @brief A function that checks the validity and safety of the target address
 *
 */
typedef BOOLEAN (*CHECK_ACCESS_VALIDITY_AND_SAFETY)(UINT64 TargetAddress, UINT32 Size);

/**
 * @brief A function that reads memory safely on the target process
 *
 */
typedef BOOLEAN (*MEMORY_MAPPER_READ_MEMORY_SAFE_ON_TARGET_PROCESS)(UINT64 VaAddressToRead, PVOID BufferToSaveMemory, SIZE_T SizeToRead);

/**
 * @brief A function that writes memory safely on the target process
 *
 */
typedef BOOLEAN (*MEMORY_MAPPER_WRITE_MEMORY_SAFE_ON_TARGET_PROCESS)(UINT64 Destination, PVOID Source, SIZE_T Size);

/**
 * @brief A function that gets the process name from the process control block
 *
 */
typedef PCHAR (*COMMON_GET_PROCESS_NAME_FROM_PROCESS_CONTROL_BLOCK)(PVOID Eprocess);

//////////////////////////////////////////////////
//			   Callback Structure               //
//////////////////////////////////////////////////

/**
 * @brief Prototype of each function needed by hypertrace module
 *
 */
typedef struct _HYPERTRACE_CALLBACKS
{
    //
    // *** Log (Hyperlog) callbacks ***
    //
    LOG_CALLBACK_PREPARE_AND_SEND_MESSAGE_TO_QUEUE LogCallbackPrepareAndSendMessageToQueueWrapper;
    LOG_CALLBACK_SEND_MESSAGE_TO_QUEUE             LogCallbackSendMessageToQueue;
    LOG_CALLBACK_SEND_BUFFER                       LogCallbackSendBuffer;
    LOG_CALLBACK_CHECK_IF_BUFFER_IS_FULL           LogCallbackCheckIfBufferIsFull;

    //
    // *** Hypervisor (Hyperhv) callbacks ***
    //
    VM_FUNC_VMX_GET_CURRENT_EXECUTION_MODE VmFuncVmxGetCurrentExecutionMode;

    //
    // *** Legacy LBR callbacks ***
    //

    VM_FUNC_CHECK_CPU_SUPPORT_FOR_SAVE_AND_LOAD_DEBUG_CONTROLS VmFuncCheckCpuSupportForSaveAndLoadDebugControls;

    VM_FUNC_GET_DEBUGCTL                       VmFuncGetDebugctl;
    VM_FUNC_GET_DEBUGCTL_VMCALL_ON_TARGET_CORE VmFuncGetDebugctlVmcallOnTargetCore;
    VM_FUNC_SET_DEBUGCTL                       VmFuncSetDebugctl;
    VM_FUNC_SET_DEBUGCTL_VMCALL_ON_TARGET_CORE VmFuncSetDebugctlVmcallOnTargetCore;

    VM_FUNC_SET_LOAD_DEBUG_CONTROLS                       VmFuncSetLoadDebugControls;
    VM_FUNC_SET_LOAD_DEBUG_CONTROLS_VMCALL_ON_TARGET_CORE VmFuncSetLoadDebugControlsVmcallOnTargetCore;
    VM_FUNC_SET_SAVE_DEBUG_CONTROLS                       VmFuncSetSaveDebugControls;
    VM_FUNC_SET_SAVE_DEBUG_CONTROLS_VMCALL_ON_TARGET_CORE VmFuncSetSaveDebugControlsVmcallOnTargetCore;

    VM_FUNC_SET_LBR_SELECT                       VmFuncSetLbrSelect;
    VM_FUNC_SET_LBR_SELECT_VMCALL_ON_TARGET_CORE VmFuncSetLbrSelectVmcallOnTargetCore;

    //
    // *** Architectural LBR callbacks ***
    //

    VM_FUNC_CHECK_CPU_SUPPORT_FOR_LOAD_AND_CLEAR_GUEST_IA32_LBR_CTL_CONTROLS VmFuncCheckCpuSupportForLoadAndClearGuestIa32LbrCtlControls;

    VM_FUNC_GET_GUEST_IA32_LBR_CTL                       VmFuncGetGuestIa32LbrCtl;
    VM_FUNC_GET_GUEST_IA32_LBR_CTL_VMCALL_ON_TARGET_CORE VmFuncGetGuestIa32LbrCtlVmcallOnTargetCore;
    VM_FUNC_SET_GUEST_IA32_LBR_CTL                       VmFuncSetGuestIa32LbrCtl;
    VM_FUNC_SET_GUEST_IA32_LBR_CTL_VMCALL_ON_TARGET_CORE VmFuncSetGuestIa32LbrCtlVmcallOnTargetCore;

    VM_FUNC_SET_LOAD_GUEST_IA32_LBR_CTL                        VmFuncSetLoadGuestIa32LbrCtl;
    VM_FUNC_SET_LOAD_GUEST_IA32_LBR_CTL_VMCALL_ON_TARGET_CORE  VmFuncSetLoadGuestIa32LbrCtlVmcallOnTargetCore;
    VM_FUNC_SET_CLEAR_GUEST_IA32_LBR_CTL                       VmFuncSetClearGuestIa32LbrCtl;
    VM_FUNC_SET_CLEAR_GUEST_IA32_LBR_CTL_VMCALL_ON_TARGET_CORE VmFuncSetClearGuestIa32LbrCtlVmcallOnTargetCore;

} HYPERTRACE_CALLBACKS, *PHYPERTRACE_CALLBACKS;
