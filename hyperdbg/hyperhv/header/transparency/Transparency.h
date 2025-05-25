/**
 * @file Transparency.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief hide the debugger from anti-debugging and anti-hypervisor methods (headers)
 * @details
 * @version 0.1
 * @date 2020-07-07
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				      Locks 	    			//
//////////////////////////////////////////////////

/**
 * @brief The lock for modifying list of process/thread for transparent-mode trap flags
 *
 */
volatile LONG TransparentModeTrapListLock;

//////////////////////////////////////////////////
//				   Definitions					//
//////////////////////////////////////////////////

/**
 * @brief IA32_TIME_STAMP_COUNTER MSR (rcx)
 *
 */
#define MSR_IA32_TIME_STAMP_COUNTER 0x10

/**
 * @brief Maximum value that can be returned by the rand function
 *
 */
#define RAND_MAX 0x7fff

/**
 * @brief maximum number of thread/process ids to be allocated for keeping track of
 * of the trap flag
 *
 */
#define MAXIMUM_NUMBER_OF_THREAD_INFORMATION_FOR_TRANSPARENT_MODE_TRAPS 500

//////////////////////////////////////////////////
//				   Structures					//
//////////////////////////////////////////////////

/**
 * @brief The measurements from user-mode and kernel-mode
 *
 */
typedef struct _TRANSPARENCY_MEASUREMENTS
{
    UINT64 CpuidAverage;
    UINT64 CpuidStandardDeviation;
    UINT64 CpuidMedian;

    UINT64 RdtscAverage;
    UINT64 RdtscStandardDeviation;
    UINT64 RdtscMedian;

    LIST_ENTRY ProcessList;

} TRANSPARENCY_MEASUREMENTS, *PTRANSPARENCY_MEASUREMENTS;

/**
 * @brief The ProcessList of TRANSPARENCY_MEASUREMENTS is from this architecture
 *
 */
typedef struct _TRANSPARENCY_PROCESS
{
    UINT32     ProcessId;
    PVOID      ProcessName;
    PVOID      BufferAddress;
    BOOLEAN    TrueIfProcessIdAndFalseIfProcessName;
    LIST_ENTRY OtherProcesses;

} TRANSPARENCY_PROCESS, *PTRANSPARENCY_PROCESS;

/**
 * @brief The thread/process information
 *
 */
typedef struct _TRANSPARENT_MODE_PROCESS_THREAD_INFORMATION
{
    union
    {
        UINT64 asUInt;

        struct
        {
            UINT32 ProcessId;
            UINT32 ThreadId;
        } Fields;
    };

} TRANSPARENT_MODE_PROCESS_THREAD_INFORMATION, *PTRANSPARENT_MODE_PROCESS_THREAD_INFORMATION;

/**
 * @brief The (optional) context parameters for the transparent-mode
 *
 */
typedef struct _TRANSPARENT_MODE_CONTEXT_PARAMS
{
    UINT64 OptionalParam1; // Optional parameter
    UINT64 OptionalParam2; // Optional parameter
    UINT64 OptionalParam3; // Optional parameter
    UINT64 OptionalParam4; // Optional parameter

} TRANSPARENT_MODE_CONTEXT_PARAMS, *PTRANSPARENT_MODE_CONTEXT_PARAMS;

/**
 * @brief The threads that we expect to get the trap flag
 *
 * @details Used for keeping track of the threads that we expect to get the trap flag
 *
 */
typedef struct _TRANSPARENT_MODE_TRAP_FLAG_STATE
{
    UINT32                                      NumberOfItems;
    TRANSPARENT_MODE_PROCESS_THREAD_INFORMATION ThreadInformation[MAXIMUM_NUMBER_OF_THREAD_INFORMATION_FOR_TRANSPARENT_MODE_TRAPS];
    UINT64                                      Context[MAXIMUM_NUMBER_OF_THREAD_INFORMATION_FOR_TRANSPARENT_MODE_TRAPS];
    TRANSPARENT_MODE_CONTEXT_PARAMS             Params[MAXIMUM_NUMBER_OF_THREAD_INFORMATION_FOR_TRANSPARENT_MODE_TRAPS];

} TRANSPARENT_MODE_TRAP_FLAG_STATE, *PTRANSPARENT_MODE_TRAP_FLAG_STATE;

//////////////////////////////////////////////////
//				   Functions					//
//////////////////////////////////////////////////

VOID
TransparentCPUID(INT32 CpuInfo[], PGUEST_REGS Regs);

BOOLEAN
TransparentSetTrapFlagAfterSyscall(VIRTUAL_MACHINE_STATE *           VCpu,
                                   UINT32                            ProcessId,
                                   UINT32                            ThreadId,
                                   UINT64                            Context,
                                   TRANSPARENT_MODE_CONTEXT_PARAMS * Params);

BOOLEAN
TransparentCheckAndHandleAfterSyscallTrapFlags(VIRTUAL_MACHINE_STATE * VCpu,
                                               UINT32                  ProcessId,
                                               UINT32                  ThreadId);

VOID
TransparentCallbackHandleAfterSyscall(VIRTUAL_MACHINE_STATE *           VCpu,
                                      UINT32                            ProcessId,
                                      UINT32                            ThreadId,
                                      UINT64                            Context,
                                      TRANSPARENT_MODE_CONTEXT_PARAMS * Params);
