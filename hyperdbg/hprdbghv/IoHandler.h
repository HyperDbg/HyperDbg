/**
 * @file IoHandler.h
 * @author Sina Karvandi (sina@rayanfam.com)
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
//                 Structures	    			//
//////////////////////////////////////////////////

/**
 * @brief exit-qualification for I/O instructions vm-exits
 * 
 */
typedef struct _IO_EXIT_QUALIFICATION
{
    UINT64 SizeOfAccess : 3;
    UINT64 AccessType : 1;
    UINT64 StringInstruction : 1;
    UINT64 RepPrefixed : 1;
    UINT64 OperandEncoding : 1;
    UINT64 Reserved : 9;
    UINT64 PortNumber : 16;
} IO_EXIT_QUALIFICATION, *PIO_EXIT_QUALIFICATION;

//////////////////////////////////////////////////
//                     Enums	    			//
//////////////////////////////////////////////////

/**
 * @brief IN Instruction or OUT Instruction
 * 
 */
typedef enum
{
    AccessOut = 0,
    AccessIn  = 1,
};

/**
 * @brief Immediate value or in DX
 * 
 */
typedef enum
{
    OpEncodingDx  = 0,
    OpEncodingImm = 1,
};

//////////////////////////////////////////////////
//        I/O Instructions Functions            //
//////////////////////////////////////////////////

unsigned char
__inbyte(unsigned short);
#pragma intrinsic(__inbyte)

inline UINT8
IoInByte(UINT16 port)
{
    return __inbyte(port);
}

unsigned short
__inword(unsigned short);
#pragma intrinsic(__inword)

inline UINT16
IoInWord(UINT16 port)
{
    return __inword(port);
}

unsigned long
__indword(unsigned short);
#pragma intrinsic(__indword)

inline UINT32
IoInDword(UINT16 port)
{
    return __indword(port);
}

void
__inbytestring(unsigned short, unsigned char *, unsigned long);
#pragma intrinsic(__inbytestring)

inline void
IoInByteString(UINT16 port, UINT8 * data, UINT32 size)
{
    __inbytestring(port, data, size);
}

void
__inwordstring(unsigned short, unsigned short *, unsigned long);
#pragma intrinsic(__inwordstring)

inline void
IoInWordString(UINT16 port, UINT16 * data, UINT32 size)
{
    __inwordstring(port, data, size);
}

void
__indwordstring(unsigned short, unsigned long *, unsigned long);
#pragma intrinsic(__indwordstring)

inline void
IoInDwordString(UINT16 port, UINT32 * data, UINT32 size)
{
    __indwordstring(port, (unsigned long *)data, size);
}

void
__outbyte(unsigned short, unsigned char);
#pragma intrinsic(__outbyte)

inline void
IoOutByte(UINT16 port, UINT8 value)
{
    __outbyte(port, value);
}

void
__outword(unsigned short, unsigned short);
#pragma intrinsic(__outword)

inline void
IoOutWord(UINT16 port, UINT16 value)
{
    __outword(port, value);
}

void
__outdword(unsigned short, unsigned long);
#pragma intrinsic(__outdword)

inline void
IoOutDword(UINT16 port, UINT32 value)
{
    __outdword(port, value);
}

void
__outbytestring(unsigned short, unsigned char *, unsigned long);
#pragma intrinsic(__outbytestring)

inline void
IoOutByteString(UINT16 port, UINT8 * data, UINT32 count)
{
    __outbytestring(port, data, count);
}

void
__outwordstring(unsigned short, unsigned short *, unsigned long);
#pragma intrinsic(__outwordstring)

inline void
IoOutWordString(UINT16 port, UINT16 * data, UINT32 count)
{
    __outwordstring(port, data, count);
}

void
__outdwordstring(unsigned short, unsigned long *, unsigned long);
#pragma intrinsic(__outdwordstring)

inline void
IoOutDwordString(UINT16 port, UINT32 * data, UINT32 count)
{
    __outdwordstring(port, (unsigned long *)data, count);
}

//////////////////////////////////////////////////
//                 Functions	    			//
//////////////////////////////////////////////////

VOID
IoHandleIoVmExits(PGUEST_REGS GuestRegs, IO_EXIT_QUALIFICATION IoQualification, RFLAGS Flags);

VOID
IoHandleIoVmExitsAndDisassemble(UINT64 GuestRip, PGUEST_REGS GuestRegs, IO_EXIT_QUALIFICATION IoQualification, RFLAGS Flags);
