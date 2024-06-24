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
 * @param VCpu The virtual processor's state
 * @param IoQualification The I/O Qualification that indicates the instruction
 * @param Flags Guest's RFLAGs
 *
 * @return VOID
 */
VOID
IoHandleIoVmExits(VIRTUAL_MACHINE_STATE * VCpu, VMX_EXIT_QUALIFICATION_IO_INSTRUCTION IoQualification, RFLAGS Flags)
{
    UINT16      Port      = 0;
    UINT32      Count     = 0;
    UINT32      Size      = 0;
    PGUEST_REGS GuestRegs = VCpu->Regs;

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
    // Take a look at :
    // https://github.com/wbenny/hvpp/blob/f1eece7d0def506f329b5770befd892497be2047/src/hvpp/hvpp/vmexit/vmexit_passthrough.cpp
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
        PortValue.AsPtr = (PVOID)(IoQualification.DirectionOfAccess == AccessIn ? GuestRegs->rdi : GuestRegs->rsi);
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
    Port = (UINT16)IoQualification.PortNumber;

    //
    // Resolve number of bytes to send/receive.
    // REP prefixed instructions always take their count
    // from *CX register.
    //
    Count = IoQualification.RepPrefixed ? (GuestRegs->rcx & 0xffffffff) : 1;

    Size = (UINT32)(IoQualification.SizeOfAccess + 1);

    switch (IoQualification.DirectionOfAccess)
    {
    case AccessIn:
        if (IoQualification.StringInstruction)
        {
            switch (Size)
            {
            case 1:
                IoInByteString(Port, (UINT8 *)PortValue.AsBytePtr, Count);
                break;
            case 2:
                IoInWordString(Port, (UINT16 *)PortValue.AsWordPtr, Count);
                break;
            case 4:
                IoInDwordString(Port, (UINT32 *)PortValue.AsDwordPtr, Count);
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
                IoOutByteString(Port, (UINT8 *)PortValue.AsBytePtr, Count);
                break;
            case 2:
                IoOutWordString(Port, (UINT16 *)PortValue.AsWordPtr, Count);
                break;
            case 4:
                IoOutDwordString(Port, (UINT32 *)PortValue.AsDwordPtr, Count);
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
 * @brief Set bits in I/O Bitmap
 *
 * @param VCpu The virtual processor's state
 * @param Port Port
 *
 * @return BOOLEAN Returns true if the I/O Bitmap is successfully applied or false if not applied
 */
BOOLEAN
IoHandleSetIoBitmap(VIRTUAL_MACHINE_STATE * VCpu, UINT32 Port)
{
    if (Port <= 0x7FFF)
    {
        SetBit(Port, (unsigned long *)VCpu->IoBitmapVirtualAddressA);
    }
    else if ((0x8000 <= Port) && (Port <= 0xFFFF))
    {
        SetBit(Port - 0x8000, (unsigned long *)VCpu->IoBitmapVirtualAddressB);
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
 *
 * @param VCpu The virtual processor's state
 * @param IoPort Port
 * @return VOID
 */
VOID
IoHandlePerformIoBitmapChange(VIRTUAL_MACHINE_STATE * VCpu, UINT32 Port)
{
    if (Port == DEBUGGER_EVENT_ALL_IO_PORTS)
    {
        //
        // Means all the bitmaps should be put to 1
        //
        memset((void *)VCpu->IoBitmapVirtualAddressA, 0xFF, PAGE_SIZE);
        memset((void *)VCpu->IoBitmapVirtualAddressB, 0xFF, PAGE_SIZE);
    }
    else
    {
        //
        // Means only one i/o bitmap is target
        //
        IoHandleSetIoBitmap(VCpu, Port);
    }
}

/**
 * @brief Reset I/O Bitmap
 * @details should be called in vmx-root mode
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
IoHandlePerformIoBitmapReset(VIRTUAL_MACHINE_STATE * VCpu)
{
    //
    // Means all the bitmaps should be put to 0
    //
    memset((void *)VCpu->IoBitmapVirtualAddressA, 0x0, PAGE_SIZE);
    memset((void *)VCpu->IoBitmapVirtualAddressB, 0x0, PAGE_SIZE);
}
