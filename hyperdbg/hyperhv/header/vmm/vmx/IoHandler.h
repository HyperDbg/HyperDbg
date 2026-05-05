/**
 * @file IoHandler.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief The I/O Handler for vm-exit headers
 * @details
 * @version 0.1
 * @date 2020-06-06
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//                     Enums	    			//
//////////////////////////////////////////////////

/**
 * @brief IN Instruction or OUT Instruction
 *
 */
typedef enum _IO_ACCESS_INSTR
{
    AccessOut = 0,
    AccessIn  = 1,
} IO_ACCESS_INSTR;

/**
 * @brief Immediate value or in DX
 *
 */
typedef enum _IO_OP_ENCODING
{
    OpEncodingDx  = 0,
    OpEncodingImm = 1,
} IO_OP_ENCODING;

//////////////////////////////////////////////////
//        I/O Instructions Functions            //
//////////////////////////////////////////////////

inline UINT8
IoInByte(UINT16 port)
{
    return CpuIoInByte(port);
}

inline UINT16
IoInWord(UINT16 port)
{
    return CpuIoInWord(port);
}

inline UINT32
IoInDword(UINT16 port)
{
    return CpuIoInDword(port);
}

inline void
IoInByteString(UINT16 port, UINT8 * data, UINT32 size)
{
    CpuIoInByteString(port, data, size);
}

inline void
IoInWordString(UINT16 port, UINT16 * data, UINT32 size)
{
    CpuIoInWordString(port, data, size);
}

inline void
IoInDwordString(UINT16 port, UINT32 * data, UINT32 size)
{
    CpuIoInDwordString(port, data, size);
}

inline void
IoOutByte(UINT16 port, UINT8 value)
{
    CpuIoOutByte(port, value);
}

inline void
IoOutWord(UINT16 port, UINT16 value)
{
    CpuIoOutWord(port, value);
}

inline void
IoOutDword(UINT16 port, UINT32 value)
{
    CpuIoOutDword(port, value);
}

inline void
IoOutByteString(UINT16 port, UINT8 * data, UINT32 count)
{
    CpuIoOutByteString(port, data, count);
}

inline void
IoOutWordString(UINT16 port, UINT16 * data, UINT32 count)
{
    CpuIoOutWordString(port, data, count);
}

inline void
IoOutDwordString(UINT16 port, UINT32 * data, UINT32 count)
{
    CpuIoOutDwordString(port, data, count);
}

//////////////////////////////////////////////////
//                 Functions	    			//
//////////////////////////////////////////////////

VOID
IoHandleIoVmExits(VIRTUAL_MACHINE_STATE * VCpu, VMX_EXIT_QUALIFICATION_IO_INSTRUCTION IoQualification, RFLAGS Flags);

VOID
IoHandlePerformIoBitmapChange(VIRTUAL_MACHINE_STATE * VCpu, UINT32 Port);

VOID
IoHandlePerformIoBitmapReset(VIRTUAL_MACHINE_STATE * VCpu);
