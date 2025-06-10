/**
 * @file HyperEvade.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief HyperDbg's SDK for hyperevade project
 * @details This file contains definitions of HyperLog routines
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
                                                                  const char * Fmt,
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

/**
 * @brief A function that sets the trap flag after a syscall
 *
 */
typedef BOOLEAN (*SYSCALL_CALLBACK_SET_TRAP_FLAG_AFTER_SYSCALL)(GUEST_REGS *                      Regs,
                                                                UINT32                            ProcessId,
                                                                UINT32                            ThreadId,
                                                                UINT64                            Context,
                                                                SYSCALL_CALLBACK_CONTEXT_PARAMS * Params);

/**
 * @brief A function that handles the trap flag
 *
 */
typedef VOID (*HV_HANDLE_TRAPFLAG)();

/**
 * @brief A function that injects a general protection (#GP)
 *
 */
typedef VOID (*EVENT_INJECT_GENERAL_PROTECTION)();

//////////////////////////////////////////////////
//			   Callback Structure               //
//////////////////////////////////////////////////

/**
 * @brief Prototype of each function needed by hyperevade module
 *
 */
typedef struct _HYPEREVADE_CALLBACKS
{
    //
    // *** Log (Hyperlog) callbacks ***
    //
    LOG_CALLBACK_PREPARE_AND_SEND_MESSAGE_TO_QUEUE LogCallbackPrepareAndSendMessageToQueueWrapper;
    LOG_CALLBACK_SEND_MESSAGE_TO_QUEUE             LogCallbackSendMessageToQueue;
    LOG_CALLBACK_SEND_BUFFER                       LogCallbackSendBuffer;
    LOG_CALLBACK_CHECK_IF_BUFFER_IS_FULL           LogCallbackCheckIfBufferIsFull;

    //
    // *** HYPEREVADE callbacks ***
    //

    //
    // Memory callbacks
    //
    CHECK_ACCESS_VALIDITY_AND_SAFETY                  CheckAccessValidityAndSafety;
    MEMORY_MAPPER_READ_MEMORY_SAFE_ON_TARGET_PROCESS  MemoryMapperReadMemorySafeOnTargetProcess;
    MEMORY_MAPPER_WRITE_MEMORY_SAFE_ON_TARGET_PROCESS MemoryMapperWriteMemorySafeOnTargetProcess;

    //
    // Common callbacks
    //
    COMMON_GET_PROCESS_NAME_FROM_PROCESS_CONTROL_BLOCK CommonGetProcessNameFromProcessControlBlock;

    //
    // System call callbacks
    //
    SYSCALL_CALLBACK_SET_TRAP_FLAG_AFTER_SYSCALL SyscallCallbackSetTrapFlagAfterSyscall;

    //
    // VMX callbacks
    //
    HV_HANDLE_TRAPFLAG              HvHandleTrapFlag;
    EVENT_INJECT_GENERAL_PROTECTION EventInjectGeneralProtection;

} HYPEREVADE_CALLBACKS, *PHYPEREVADE_CALLBACKS;
