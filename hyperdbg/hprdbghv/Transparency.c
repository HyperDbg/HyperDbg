/**
 * @file Transparency.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief try to hide the debugger from anti-debugging and anti-hypervisor methods
 * @details
 * @version 0.1
 * @date 2020-07-07
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"
/**
 * @brief maximum random value
 */
#define MY_RAND_MAX 32768

/**
 * @brief pre-defined log result
 * @details we used this because we want to avoid using floating-points in
 * kernel
 */
int TransparentTableLog[] =
    {
        0,
        69,
        110,
        139,
        161,
        179,
        195,
        208,
        220,
        230,
        240,
        248,
        256,
        264,
        271,
        277,
        283,
        289,
        294,
        300,
        304,
        309,
        314,
        318,
        322,
        326,
        330,
        333,
        337,
        340,
        343,
        347,
        350,
        353,
        356,
        358,
        361,
        364,
        366,
        369,
        371,
        374,
        376,
        378,
        381,
        383,
        385,
        387,
        389,
        391,
        393,
        395,
        397,
        399,
        401,
        403,
        404,
        406,
        408,
        409,
        411,
        413,
        414,
        416,
        417,
        419,
        420,
        422,
        423,
        425,
        426,
        428,
        429,
        430,
        432,
        433,
        434,
        436,
        437,
        438,
        439,
        441,
        442,
        443,
        444,
        445,
        447,
        448,
        449,
        450,
        451,
        452,
        453,
        454,
        455,
        456,
        457,
        458,
        460,
        461};

/**
 * @brief Generate a random number by utilizing RDTSC instruction.
 *
 * Masking 16 LSB of the measured clock time.
 * @return UINT32 
 */
UINT32
TransparentGetRand()
{
    ULONG64 Tsc;
    UINT32  Rand;

    Tsc  = __rdtsc();
    Rand = Tsc & 0xffff;

    return Rand;
}

/**
 * @brief Integer power function definition.
 *
 * @params x Base Value 
 * @params p Power Value 
 * @return int 
 */
int
TransparentPow(int x, int p)
{
    int Res = 1;
    for (int i = 0; i < p; i++)
    {
        Res = Res * x;
    }
    return Res;
}

/**
 * @brief Integer Natural Logarithm function estimation.
 *
 * @params x input value 
 * @return int 
 */
int
TransparentLog(int x)
{
    int n     = x;
    int Digit = 0;

    while (n >= 100)
    {
        n = n / 10;
        Digit++;
    }

    //
    //Use pre-defined values of logarithms and estimate the total value
    //
    return TransparentTableLog[n] / 100 + (Digit * 23) / 10;
}
/**
 * @brief Integer root function estimation.
 *
 * @params x input value 
 * @return int 
 */
int
TransparentSqrt(int x)
{
    int Res = 0;
    int Bit;

    //
    // The second-to-top bit is set.
    //
    Bit = 1 << 30;

    //
    // "Bit" starts at the highest power of four <= the argument.
    //
    while (Bit > x)
        Bit >>= 2;

    while (Bit != 0)
    {
        if (x >= Res + Bit)
        {
            x -= Res + Bit;
            Res = (Res >> 1) + Bit;
        }
        else
            Res >>= 1;
        Bit >>= 2;
    }
    return Res;
}

/**
 * @brief Integer Gaussian Random Number Generator(GRNG) based on Box-Muller method. A Float to Integer 
 * mapping is used in the function.
 *
 * @params Average Mean 
 * @parans Sigma Standard Deviation of the targeted Gaussian Distribution
 * @return int 
 */
int
TransparentRandn(int Average, int Sigma)
{
    int U1, r1, U2, r2, W, Mult;
    int X1, X2 = 0, XS1;
    int State   = 0;
    int LogTemp = 0;

    if (State == 1)
    {
        State = !State;
        return (Average + Sigma * X2);
    }

    do
    {
        r1 = TransparentGetRand();
        r2 = TransparentGetRand();

        U1 = (r1 % MY_RAND_MAX) - (MY_RAND_MAX / 2);

        U2 = (r2 % MY_RAND_MAX) - (MY_RAND_MAX / 2);

        W = U1 * U1 + U2 * U2;
    } while (W >= MY_RAND_MAX * MY_RAND_MAX / 2 || W == 0);

    LogTemp = (TransparentLog(W) - TransparentLog(MY_RAND_MAX * MY_RAND_MAX));

    Mult = TransparentSqrt((-2 * LogTemp) * (MY_RAND_MAX * MY_RAND_MAX / W));

    X1  = U1 * Mult / MY_RAND_MAX;
    XS1 = U1 * Mult;

    X2 = U2 * Mult / MY_RAND_MAX;

    State = !State;

    return (Average + (Sigma * XS1) / MY_RAND_MAX);
}

/**
 * @brief Add name or process id of the target process to the list
 * of processes that HyperDbg should apply transparent-mode on them
 * 
 * @param Measurements 
 * @return BOOLEAN 
 */
BOOLEAN
TransparentAddNameOrProcessIdToTheList(PDEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE Measurements)
{
    SIZE_T                SizeOfBuffer;
    PTRANSPARENCY_PROCESS PidAndNameBuffer;

    //
    // Check whether it's a process id or it's a process name
    //
    if (Measurements->TrueIfProcessIdAndFalseIfProcessName)
    {
        //
        // It's a process Id
        //
        SizeOfBuffer = sizeof(TRANSPARENCY_PROCESS);
    }
    else
    {
        //
        // It's a process name
        //
        SizeOfBuffer = sizeof(TRANSPARENCY_PROCESS) + Measurements->LengthOfProcessName;
    }

    //
    // Allocate the Buffer
    //
    PidAndNameBuffer = ExAllocatePoolWithTag(NonPagedPool, SizeOfBuffer, POOLTAG);

    if (PidAndNameBuffer == NULL)
    {
        return FALSE;
    }

    //
    // Zero the memory
    //
    RtlZeroMemory(PidAndNameBuffer, SizeOfBuffer);

    //
    // Save the address of the buffer for future de-allocation
    //
    PidAndNameBuffer->BufferAddress = PidAndNameBuffer;

    //
    // Check again whether it's a process id or it's a process name
    // then fill the structure
    //
    if (Measurements->TrueIfProcessIdAndFalseIfProcessName)
    {
        //
        // It's a process Id
        //
        PidAndNameBuffer->ProcessId                            = Measurements->ProcId;
        PidAndNameBuffer->TrueIfProcessIdAndFalseIfProcessName = TRUE;
    }
    else
    {
        //
        // It's a process name
        //
        PidAndNameBuffer->TrueIfProcessIdAndFalseIfProcessName = FALSE;

        //
        // Move the process name string to the end of the buffer
        //
        RtlCopyBytes((UINT64)PidAndNameBuffer + sizeof(TRANSPARENCY_PROCESS),
                     (UINT64)Measurements + sizeof(DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE),
                     Measurements->LengthOfProcessName);

        //
        // Set the process name location
        //
        PidAndNameBuffer->ProcessName = (UINT64)PidAndNameBuffer + sizeof(TRANSPARENCY_PROCESS);
    }

    //
    // Link it to the list of process that we need to transparent
    // vm-exits for them
    //
    InsertHeadList(&g_TransparentModeMeasurements->ProcessList, &(PidAndNameBuffer->OtherProcesses));
}

/**
 * @brief Hide debugger on transparent-mode (activate transparent-mode)
 * 
 * @param Measurements 
 * @return NTSTATUS 
 */
NTSTATUS
TransparentHideDebugger(PDEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE Measurements)
{
    //
    // Check whether the transparent-mode was already initialized or not
    //
    if (!g_TransparentMode)
    {
        //
        // Allocate the measurements buffer
        //
        g_TransparentModeMeasurements = (PTRANSPARENCY_MEASUREMENTS)ExAllocatePoolWithTag(NonPagedPool,
                                                                                          sizeof(TRANSPARENCY_MEASUREMENTS),
                                                                                          POOLTAG);
        if (!g_TransparentModeMeasurements)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Zero the memory
        //
        RtlZeroMemory(g_TransparentModeMeasurements, sizeof(TRANSPARENCY_MEASUREMENTS));

        //
        // Initialize the lists
        //
        InitializeListHead(&g_TransparentModeMeasurements->ProcessList);

        //
        // Fill the transparency details CPUID
        //
        g_TransparentModeMeasurements->CpuidAverage           = Measurements->CpuidAverage;
        g_TransparentModeMeasurements->CpuidMedian            = Measurements->CpuidMedian;
        g_TransparentModeMeasurements->CpuidStandardDeviation = Measurements->CpuidStandardDeviation;

        //
        // Fill the transparency details RDTSC
        //
        g_TransparentModeMeasurements->RdtscAverage           = Measurements->RdtscAverage;
        g_TransparentModeMeasurements->RdtscMedian            = Measurements->RdtscMedian;
        g_TransparentModeMeasurements->RdtscStandardDeviation = Measurements->RdtscStandardDeviation;

        //
        // add the new process name or Id to the list
        //
        TransparentAddNameOrProcessIdToTheList(Measurements);

        //
        // Enable RDTSC and RDTSCP exiting on all cores
        //
        ExtensionCommandEnableRdtscExitingAllCores();

        //
        // Finally, enable the transparent-mode
        //
        g_TransparentMode = TRUE;
    }
    else
    {
        //
        // It's already initialized, we just need to
        // add the new process name or Id to the list
        //
        TransparentAddNameOrProcessIdToTheList(Measurements);
    }

    return STATUS_SUCCESS;
}

/**
 * @brief Deactive transparent-mode
 * 
 * @return NTSTATUS 
 */
NTSTATUS
TransparentUnhideDebugger()
{
    PLIST_ENTRY TempList           = 0;
    PVOID       BufferToDeAllocate = 0;

    if (g_TransparentMode)
    {
        //
        // Disable the transparent-mode
        //
        g_TransparentMode = FALSE;

        //
        // Disable RDTSC and RDTSCP emulation
        //
        ExtensionCommandDisableRdtscExitingAllCores();

        //
        // Free list of allocated buffers
        //
        // Check for process id and process name, if not match then we don't emulate it
        //
        TempList = &g_TransparentModeMeasurements->ProcessList;
        while (&g_TransparentModeMeasurements->ProcessList != TempList->Flink)
        {
            TempList                             = TempList->Flink;
            PTRANSPARENCY_PROCESS ProcessDetails = (PTRANSPARENCY_PROCESS)CONTAINING_RECORD(TempList, TRANSPARENCY_PROCESS, OtherProcesses);

            //
            // Save the buffer so we can de-allocate it
            //
            BufferToDeAllocate = ProcessDetails->BufferAddress;

            //
            // We have to remove the event from the list
            //
            RemoveEntryList(&ProcessDetails->OtherProcesses);

            //
            // Free the buffer
            //
            ExFreePoolWithTag(BufferToDeAllocate, POOLTAG);
        }

        //
        // Deallocate the measurements buffer
        //
        ExFreePoolWithTag(g_TransparentModeMeasurements, POOLTAG);

        return STATUS_SUCCESS;
    }
    else
    {
        return STATUS_UNSUCCESSFUL;
    }
}

/**
 * @brief VM-Exit handler for different exit reasons 
 * @details Should be called from vmx-root
 * 
 * @param GuestRegs Registers that are automatically saved by AsmVmexitHandler (HOST_RIP)
 * @param ProcessorIndex Processor Index
 * @param ExitReason Exit Reason
 * @return BOOLEAN Return True we should emulate RDTSCP
 *  or return false if we should not emulate RDTSCP
 */
BOOLEAN
TransparentModeStart(PGUEST_REGS GuestRegs, ULONG ProcessorIndex, UINT32 ExitReason)
{
    int         Aux                = 0;
    ULONG64     GuestCsSel         = 0;
    PLIST_ENTRY TempList           = 0;
    PCHAR       CurrentProcessName = 0;
    PCHAR       CurrentProcessId;
    ULONG64     CurrrentTime;
    HANDLE      CurrentThreadId;
    BOOLEAN     Result                      = TRUE;
    BOOLEAN     IsProcessOnTransparencyList = FALSE;

    //
    // Save the current time
    //
    CurrrentTime = __rdtscp(&Aux);

    //
    // Save time of vm-exit on each logical processor separately
    //
    g_GuestState[ProcessorIndex].TransparencyState.PreviousTimeStampCounter = CurrrentTime;

    //
    // Find the current process id and name
    //
    CurrentProcessId   = PsGetCurrentProcessId();
    CurrentProcessName = GetProcessNameFromEprocess(PsGetCurrentProcess());

    //
    // Check for process id and process name, if not match then we don't emulate it
    //
    TempList = &g_TransparentModeMeasurements->ProcessList;
    while (&g_TransparentModeMeasurements->ProcessList != TempList->Flink)
    {
        TempList                             = TempList->Flink;
        PTRANSPARENCY_PROCESS ProcessDetails = (PTRANSPARENCY_PROCESS)CONTAINING_RECORD(TempList, TRANSPARENCY_PROCESS, OtherProcesses);
        if (ProcessDetails->TrueIfProcessIdAndFalseIfProcessName)
        {
            //
            // This entry is process id
            //
            if (ProcessDetails->ProcessId == CurrentProcessId)
            {
                //
                // Let the transparency handler to handle it
                //
                IsProcessOnTransparencyList = TRUE;
                break;
            }
        }
        else
        {
            //
            // This entry is a process name
            //
            if (CurrentProcessName != NULL && StartsWith(CurrentProcessName, ProcessDetails->ProcessName))
            {
                //
                // Let the transparency handler to handle it
                //
                IsProcessOnTransparencyList = TRUE;
                break;
            }
        }
    }

    //
    // Check whether we find this process on transparency list or not
    //
    if (!IsProcessOnTransparencyList)
    {
        //
        // No, we didn't let's do the normal tasks
        //
        return TRUE;
    }

    //
    // Get current thread Id
    //
    CurrentThreadId = PsGetCurrentThreadId();

    //
    // Check whether we are in new thread or in previous thread
    //
    if (g_GuestState[ProcessorIndex].TransparencyState.ThreadId != CurrentThreadId)
    {
        //
        // It's a new thread Id reset everything
        //
        g_GuestState[ProcessorIndex].TransparencyState.ThreadId                        = CurrentThreadId;
        g_GuestState[ProcessorIndex].TransparencyState.RevealedTimeStampCounterByRdtsc = NULL;
        g_GuestState[ProcessorIndex].TransparencyState.CpuidAfterRdtscDetected         = FALSE;
    }

    //
    // Now, it's time to check and play with RDTSC/P and CPUID
    //

    if (ExitReason == EXIT_REASON_RDTSC || ExitReason == EXIT_REASON_RDTSCP)
    {
        if (g_GuestState[ProcessorIndex].TransparencyState.RevealedTimeStampCounterByRdtsc == NULL)
        {
            //
            // It's a timing and the previous time for the thread is null
            // so we need to save the time (maybe) for future use
            //
            g_GuestState[ProcessorIndex].TransparencyState.RevealedTimeStampCounterByRdtsc = CurrrentTime;
        }
        else if (g_GuestState[ProcessorIndex].TransparencyState.CpuidAfterRdtscDetected == TRUE)
        {
            //
            // Someone tries to know about the hypervisor
            // let's play with them
            //

            // LogInfo("Possible RDTSC+CPUID+RDTSC");
        }
        else if (g_GuestState[ProcessorIndex].TransparencyState.RevealedTimeStampCounterByRdtsc != NULL &&
                 g_GuestState[ProcessorIndex].TransparencyState.CpuidAfterRdtscDetected == FALSE)
        {
            //
            // It's a new rdtscp, let's save the new value
            //
            g_GuestState[ProcessorIndex].TransparencyState.RevealedTimeStampCounterByRdtsc +=
                TransparentRandn(g_TransparentModeMeasurements->CpuidAverage,
                                 g_TransparentModeMeasurements->CpuidStandardDeviation);
            ;
        }

        //
        // Adjust the rdtsc based on RevealedTimeStampCounterByRdtsc
        //
        GuestRegs->rax = 0x00000000ffffffff &
                         g_GuestState[ProcessorIndex].TransparencyState.RevealedTimeStampCounterByRdtsc;

        GuestRegs->rdx = 0x00000000ffffffff &
                         (g_GuestState[ProcessorIndex].TransparencyState.RevealedTimeStampCounterByRdtsc >> 32);

        //
        // Check if we need to adjust rcx as a result of rdtscp
        //
        if (ExitReason == EXIT_REASON_RDTSCP)
        {
            GuestRegs->rcx = 0x00000000ffffffff & Aux;
        }
        //
        // Shows that vm-exit handler should not emulate the RDTSC/P
        //
        Result = FALSE;
    }
    else if (ExitReason == EXIT_REASON_CPUID &&
             g_GuestState[ProcessorIndex].TransparencyState.RevealedTimeStampCounterByRdtsc != NULL)
    {
        //
        // The guy executed one or more CPUIDs after an rdtscp so we
        //  need to add new cpuid value to previous timer and also
        //  we need to store it somewhere to remeber this behavior
        //
        g_GuestState[ProcessorIndex].TransparencyState.RevealedTimeStampCounterByRdtsc +=
            TransparentRandn(g_TransparentModeMeasurements->CpuidAverage,
                             g_TransparentModeMeasurements->CpuidStandardDeviation);

        g_GuestState[ProcessorIndex].TransparencyState.CpuidAfterRdtscDetected = TRUE;
    }

    return Result;
}
