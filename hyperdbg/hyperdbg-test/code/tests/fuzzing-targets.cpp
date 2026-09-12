/**
 * @file fuzzing-targets.cpp
 * @author HyperDbg Dev Team
 * @brief Reference Ring-0 and Ring-3 vulnerability targets for snapshot fuzzing verification.
 * @details Provides concrete target functions simulating user-mode file/packet parsers
 *          and kernel driver IOCTL dispatch handlers for end-to-end LibAFL snapshot testing.
 * @version 0.1
 * @date 2026-09-10
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#pragma pack(push, 1)
typedef struct _MOCK_PARSER_HEADER
{
    UINT16 Magic;       // Expected 0x5A5A ('ZZ')
    UINT16 Command;     // 1 = Echo, 2 = Transform, 0xFF = Crash Trigger
    UINT32 DataLength;  // Payload byte count
} MOCK_PARSER_HEADER, *PMOCK_PARSER_HEADER;
#pragma pack(pop)

/**
 * @brief Ring-3 Target: Simulates a user-mode binary parser susceptible to memory corruption.
 *
 * @param Buffer Raw input testcase buffer.
 * @param Size Length of testcase in bytes.
 * @return INT32 Parsing exit code (0 = success, -1 = invalid format, -2 = exception triggered).
 */
INT32
FuzzTargetUserModeParser(const UINT8 * Buffer, SIZE_T Size)
{
    if (Buffer == NULL || Size < sizeof(MOCK_PARSER_HEADER))
    {
        return -1;
    }

    const MOCK_PARSER_HEADER * Header = (const MOCK_PARSER_HEADER *)Buffer;

    // Check magic signature
    if (Header->Magic != 0x5A5A)
    {
        return -1;
    }

    // Branch 1: Normal echo
    if (Header->Command == 1)
    {
        volatile UINT32 Sum = 0;
        SIZE_T CopyLen = min((SIZE_T)Header->DataLength, Size - sizeof(MOCK_PARSER_HEADER));
        for (SIZE_T i = 0; i < CopyLen; i++)
        {
            Sum += Buffer[sizeof(MOCK_PARSER_HEADER) + i];
        }
        return 0;
    }

    // Branch 2: Transform
    if (Header->Command == 2)
    {
        volatile UINT8 Scratch[32] = {0};
        SIZE_T CopyLen = min((SIZE_T)Header->DataLength, sizeof(Scratch));
        for (SIZE_T i = 0; i < CopyLen; i++)
        {
            Scratch[i] = Buffer[sizeof(MOCK_PARSER_HEADER) + i] ^ 0xAA;
        }
        return (INT32)Scratch[0];
    }

    // Branch 3: Trigger crash condition (Simulated vulnerability for fuzzer detection)
    if (Header->Command == 0xFF)
    {
        // Null-pointer dereference
        return -2;
    }

    return 0;
}

/**
 * @brief Ring-0 Target: Simulates a kernel IOCTL dispatch handler susceptible to kernel fault.
 *
 * @param IoctlCode Simulated IOCTL code.
 * @param InputBuffer Input payload.
 * @param InputSize Length of input.
 * @param OutStatus Output status code.
 * @return BOOLEAN TRUE on dispatch success.
 */
BOOLEAN
FuzzTargetKernelIoctlHandler(UINT32 IoctlCode, const UINT8 * InputBuffer, SIZE_T InputSize, UINT32 * OutStatus)
{
    if (OutStatus == NULL)
    {
        return FALSE;
    }

    *OutStatus = 0; // STATUS_SUCCESS

    if (InputBuffer == NULL || InputSize == 0)
    {
        *OutStatus = 0xC000000D; // STATUS_INVALID_PARAMETER
        return FALSE;
    }

    switch (IoctlCode)
    {
    case 0x80002000: // IOCTL_MOCK_QUERY
        if (InputSize >= 4 && InputBuffer[0] == 'P' && InputBuffer[1] == 'I' && InputBuffer[2] == 'N' && InputBuffer[3] == 'G')
        {
            *OutStatus = 0;
            return TRUE;
        }
        *OutStatus = 0xC0000023; // STATUS_BUFFER_TOO_SMALL
        return FALSE;

    case 0x80002004: // IOCTL_MOCK_MUTATE_REGISTER
        if (InputSize >= 8)
        {
            UINT64 Value = *(const UINT64 *)InputBuffer;
            if (Value == 0xDEADBEEFCAFEBABEULL)
            {
                // Vulnerability trigger: Kernel access violation simulated
                *OutStatus = 0xC0000005; // STATUS_ACCESS_VIOLATION
                return FALSE;
            }
            *OutStatus = 0;
            return TRUE;
        }
        return FALSE;

    default:
        *OutStatus = 0xC0000002; // STATUS_NOT_IMPLEMENTED
        return FALSE;
    }
}
