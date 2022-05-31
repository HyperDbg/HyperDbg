/**
 * @file IoHandler.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief The I/O Handler for vm-exit
 * @details 
 * @version 0.1
 * @date 2020-06-06
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "pch.h"

/**
 * @brief VM-Exit handler for I/O Instructions (in/out)
 * 
 * @param GuestRegs Registers that are automatically saved by AsmVmexitHandler
 * @param IoQualification The I/O Qualification that indicates the instruction
 * @param Flags Guest's RFLAGs
 *
 * @return VOID
 */
VOID
IoHandleIoVmExits(PGUEST_REGS GuestRegs, VMX_EXIT_QUALIFICATION_IO_INSTRUCTION IoQualification, RFLAGS Flags)
{
    UINT16 Port  = 0;
    UINT32 Count = 0;
    UINT32 Size  = 0;
    UINT64 GpReg = 0;

    //
    // VMWare tools uses port  (port 0x5658/0x5659) as I/O backdoor
    // This function will not handle these cases so if you put bitmap
    // to cause vm-exit on port 0x5658/0x5659 then VMWare tools will
    // crash
    //

    union
    {
        unsigned char *  AsBytePtr;
        unsigned short * AsWordPtr;
        unsigned long *  AsDwordPtr;

        void * AsPtr;
        UINT64 AsUint64;

    } PortValue;

    //
    // The I/O Implementation is derived from Petr Benes's hvpp
    // Take a look at : https://github.com/wbenny/hvpp/blob/f1eece7d0def506f329b5770befd892497be2047/src/hvpp/hvpp/vmexit/vmexit_passthrough.cpp
    //

    //
    // We don't check if CPL == 0 here, because the CPU would
    // raise #GP instead of VM-exit.
    //
    // See Vol3C[25.1.1(Relative Priority of Faults and VM Exits)]
    //

    //
    // Resolve address of the source or destination.
    //
    if (IoQualification.StringInstruction)
    {
        //
        // String operations always operate either on RDI (in) or
        // RSI (out) registers.
        //
        PortValue.AsPtr = IoQualification.DirectionOfAccess == AccessIn ? GuestRegs->rdi : GuestRegs->rsi;
    }
    else
    {
        //
        // Save pointer to the RAX register.
        //
        PortValue.AsPtr = &GuestRegs->rax;
    }

    //
    // Resolve port as a nice 16-bit number.
    //
    Port = IoQualification.PortNumber;

    //
    // Resolve number of bytes to send/receive.
    // REP prefixed instructions always take their count
    // from *CX register.
    //
    Count = IoQualification.RepPrefixed ? (GuestRegs->rcx & 0xffffffff) : 1;

    Size = IoQualification.SizeOfAccess + 1;

    switch (IoQualification.DirectionOfAccess)
    {
    case AccessIn:
        if (IoQualification.StringInstruction)
        {
            switch (Size)
            {
            case 1:
                IoInByteString(Port, PortValue.AsBytePtr, Count);
                break;
            case 2:
                IoInWordString(Port, PortValue.AsWordPtr, Count);
                break;
            case 4:
                IoInDwordString(Port, PortValue.AsDwordPtr, Count);
                break;
            }
        }
        else
        {
            //
            // Note that port_value holds pointer to the
            // vp.context().rax member, therefore we're
            // directly overwriting the RAX value.
            //
            switch (Size)
            {
            case 1:
                *PortValue.AsBytePtr = IoInByte(Port);
                break;
            case 2:
                *PortValue.AsWordPtr = IoInWord(Port);
                break;
            case 4:
                *PortValue.AsDwordPtr = IoInDword(Port);
                break;
            }
        }
        break;

    case AccessOut:
        if (IoQualification.StringInstruction)
        {
            switch (Size)
            {
            case 1:
                IoOutByteString(Port, PortValue.AsBytePtr, Count);
                break;
            case 2:
                IoOutWordString(Port, PortValue.AsWordPtr, Count);
                break;
            case 4:
                IoOutDwordString(Port, PortValue.AsDwordPtr, Count);
                break;
            }
        }
        else
        {
            //
            // Note that port_value holds pointer to the
            // vp.context().rax member, therefore we're
            // directly reading from the RAX value.
            //
            switch (Size)
            {
            case 1:
                IoOutByte(Port, *PortValue.AsBytePtr);
                break;
            case 2:
                IoOutWord(Port, *PortValue.AsWordPtr);
                break;
            case 4:
                IoOutDword(Port, *PortValue.AsDwordPtr);
                break;
            }
        }
        break;
    }

    if (IoQualification.StringInstruction)
    {
        //
        // Update register:
        // If the DF (direction flag) is set, decrement,
        // otherwise increment.
        //
        // For in the register is RDI, for out it's RSI.
        //
        UINT64 GpReg = IoQualification.DirectionOfAccess == AccessIn ? GuestRegs->rdi : GuestRegs->rsi;

        if (Flags.DirectionFlag)
        {
            GpReg -= Count * Size;
        }
        else
        {
            GpReg += Count * Size;
        }

        //
        // We've sent/received everything, reset counter register
        // to 0.
        //
        if (IoQualification.RepPrefixed)
        {
            GuestRegs->rcx = 0;
        }
    }
}

/**
 * @brief VM-Exit handler for I/O Instructions (in/out) and show the disassembly code
 * 
 * @param GuestRip Guest's IP
 * @param GuestRegs Registers that are automatically saved by AsmVmexitHandler
 * @param IoQualification The I/O Qualification that indicates the instruction
 * @param Flags Guest's RFLAGs
 *
 * @return VOID
 */
VOID
IoHandleIoVmExitsAndDisassemble(UINT64 GuestRip, PGUEST_REGS GuestRegs, VMX_EXIT_QUALIFICATION_IO_INSTRUCTION IoQualification, RFLAGS Flags)
{
    CR3_TYPE GuestCr3;
    UINT64   OriginalCr3;
    size_t   InstructionLength;

    //
    // Find the current process's running cr3
    //
    GuestCr3.Flags = GetRunningCr3OnTargetProcess().Flags;

    OriginalCr3 = __readcr3();
    __writecr3(GuestCr3.Flags);

    //
    // Read the memory
    //

    //
    // Maximum instructions length in AMD64 (16) + Size Of Intruction (4) + RAX (8) + RDX (8)
    //
    CHAR * InstructionBuffer[16] = {0};
    MemoryMapperReadMemorySafe(GuestRip, InstructionBuffer, 16);
    __writecr3(OriginalCr3);

    //
    // Detect length of the instruction
    //
    InstructionLength = ldisasm(((UINT64)InstructionBuffer), TRUE);

    //
    // Handle the I/O Instruction
    //
    IoHandleIoVmExits(GuestRegs, IoQualification, Flags);
}

/**
 * @brief Set bits in I/O Bitmap
 * 
 * @param Port Port
 * @param ProcessorID Processor ID
 * @return BOOLEAN Returns true if the I/O Bitmap is succcessfully applied or false if not applied
 */
BOOLEAN
IoHandleSetIoBitmap(UINT64 Port, UINT32 ProcessorID)
{
    VIRTUAL_MACHINE_STATE * CurrentVmState = &g_GuestState[ProcessorID];

    if (Port <= 0x7FFF)
    {
        SetBit(Port, CurrentVmState->IoBitmapVirtualAddressA);
    }
    else if ((0x8000 <= Port) && (Port <= 0xFFFF))
    {
        SetBit(Port - 0x8000, CurrentVmState->IoBitmapVirtualAddressB);
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Change I/O Bitmap 
 * @details should be called in vmx-root mode
 * @param IoPort Port 
 * @return VOID 
 */
VOID
IoHandlePerformIoBitmapChange(UINT64 Port)
{
    UINT32                  CoreIndex      = KeGetCurrentProcessorNumber();
    VIRTUAL_MACHINE_STATE * CurrentVmState = &g_GuestState[CoreIndex];

    if (Port == DEBUGGER_EVENT_ALL_IO_PORTS)
    {
        //
        // Means all the bitmaps should be put to 1
        //
        memset(CurrentVmState->IoBitmapVirtualAddressA, 0xFF, PAGE_SIZE);
        memset(CurrentVmState->IoBitmapVirtualAddressB, 0xFF, PAGE_SIZE);
    }
    else
    {
        //
        // Means only one i/o bitmap is target
        //
        IoHandleSetIoBitmap(Port, CoreIndex);
    }
}

/**
 * @brief Reset I/O Bitmap 
 * @details should be called in vmx-root mode
 * @return VOID 
 */
VOID
IoHandlePerformIoBitmapReset()
{
    UINT32                  CoreIndex      = KeGetCurrentProcessorNumber();
    VIRTUAL_MACHINE_STATE * CurrentVmState = &g_GuestState[CoreIndex];
    //
    // Means all the bitmaps should be put to 0
    //
    memset(CurrentVmState->IoBitmapVirtualAddressA, 0x0, PAGE_SIZE);
    memset(CurrentVmState->IoBitmapVirtualAddressB, 0x0, PAGE_SIZE);
}
