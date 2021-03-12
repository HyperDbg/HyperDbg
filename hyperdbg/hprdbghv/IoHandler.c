/**
 * @file IoHandler.c
 * @author Sina Karvandi (sina@rayanfam.com)
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
IoHandleIoVmExits(PGUEST_REGS GuestRegs, IO_EXIT_QUALIFICATION IoQualification, RFLAGS Flags)
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
        PortValue.AsPtr = IoQualification.AccessType == AccessIn ? GuestRegs->rdi : GuestRegs->rsi;
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

    switch (IoQualification.AccessType)
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
        UINT64 GpReg = IoQualification.AccessType == AccessIn ? GuestRegs->rdi : GuestRegs->rsi;

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
IoHandleIoVmExitsAndDisassemble(UINT64 GuestRip, PGUEST_REGS GuestRegs, IO_EXIT_QUALIFICATION IoQualification, RFLAGS Flags)
{
    UINT64 GuestCr3;
    UINT64 OriginalCr3;
    size_t InstructionLength;

    //
    // Due to KVA Shadowing, we need to switch to a different directory table base
    // if the PCID indicates this is a user mode directory table base.
    //

    NT_KPROCESS * CurrentProcess = (NT_KPROCESS *)(PsGetCurrentProcess());
    GuestCr3                     = CurrentProcess->DirectoryTableBase;

    OriginalCr3 = __readcr3();
    __writecr3(GuestCr3);

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
