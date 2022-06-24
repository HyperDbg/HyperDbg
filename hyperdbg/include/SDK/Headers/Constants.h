/**
 * @file Constants.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief HyperDbg's SDK constants
 * @details This file contains definitions of constants
 * used in HyperDbg
 * @version 0.2
 * @date 2022-06-24
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//			 Version Information                //
//////////////////////////////////////////////////

#define VERSION_MAJOR 0
#define VERSION_MINOR 2
#define VERSION_PATCH 0

//
// Example of __DATE__ string: "Jul 27 2012"
//                              01234567890

#define BUILD_YEAR_CH0 (__DATE__[7])
#define BUILD_YEAR_CH1 (__DATE__[8])
#define BUILD_YEAR_CH2 (__DATE__[9])
#define BUILD_YEAR_CH3 (__DATE__[10])

#define BUILD_MONTH_IS_JAN (__DATE__[0] == 'J' && __DATE__[1] == 'a' && __DATE__[2] == 'n')
#define BUILD_MONTH_IS_FEB (__DATE__[0] == 'F')
#define BUILD_MONTH_IS_MAR (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'r')
#define BUILD_MONTH_IS_APR (__DATE__[0] == 'A' && __DATE__[1] == 'p')
#define BUILD_MONTH_IS_MAY (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'y')
#define BUILD_MONTH_IS_JUN (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'n')
#define BUILD_MONTH_IS_JUL (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'l')
#define BUILD_MONTH_IS_AUG (__DATE__[0] == 'A' && __DATE__[1] == 'u')
#define BUILD_MONTH_IS_SEP (__DATE__[0] == 'S')
#define BUILD_MONTH_IS_OCT (__DATE__[0] == 'O')
#define BUILD_MONTH_IS_NOV (__DATE__[0] == 'N')
#define BUILD_MONTH_IS_DEC (__DATE__[0] == 'D')

#define BUILD_MONTH_CH0 \
    ((BUILD_MONTH_IS_OCT || BUILD_MONTH_IS_NOV || BUILD_MONTH_IS_DEC) ? '1' : '0')

#define BUILD_MONTH_CH1                                         \
    (                                                           \
        (BUILD_MONTH_IS_JAN) ? '1' : (BUILD_MONTH_IS_FEB) ? '2' \
                                 : (BUILD_MONTH_IS_MAR)   ? '3' \
                                 : (BUILD_MONTH_IS_APR)   ? '4' \
                                 : (BUILD_MONTH_IS_MAY)   ? '5' \
                                 : (BUILD_MONTH_IS_JUN)   ? '6' \
                                 : (BUILD_MONTH_IS_JUL)   ? '7' \
                                 : (BUILD_MONTH_IS_AUG)   ? '8' \
                                 : (BUILD_MONTH_IS_SEP)   ? '9' \
                                 : (BUILD_MONTH_IS_OCT)   ? '0' \
                                 : (BUILD_MONTH_IS_NOV)   ? '1' \
                                 : (BUILD_MONTH_IS_DEC)   ? '2' \
                                                          : /* error default */ '?')

#define BUILD_DAY_CH0 ((__DATE__[4] >= '0') ? (__DATE__[4]) : '0')
#define BUILD_DAY_CH1 (__DATE__[5])

//
// Example of __TIME__ string: "21:06:19"
//                              01234567

#define BUILD_HOUR_CH0 (__TIME__[0])
#define BUILD_HOUR_CH1 (__TIME__[1])

#define BUILD_MIN_CH0 (__TIME__[3])
#define BUILD_MIN_CH1 (__TIME__[4])

#define BUILD_SEC_CH0 (__TIME__[6])
#define BUILD_SEC_CH1 (__TIME__[7])

#if VERSION_MAJOR > 100

#    define VERSION_MAJOR_INIT                    \
        ((VERSION_MAJOR / 100) + '0'),            \
            (((VERSION_MAJOR % 100) / 10) + '0'), \
            ((VERSION_MAJOR % 10) + '0')

#elif VERSION_MAJOR > 10

#    define VERSION_MAJOR_INIT        \
        ((VERSION_MAJOR / 10) + '0'), \
            ((VERSION_MAJOR % 10) + '0')

#else

#    define VERSION_MAJOR_INIT \
        (VERSION_MAJOR + '0')

#endif

#if VERSION_MINOR > 100

#    define VERSION_MINOR_INIT                    \
        ((VERSION_MINOR / 100) + '0'),            \
            (((VERSION_MINOR % 100) / 10) + '0'), \
            ((VERSION_MINOR % 10) + '0')

#elif VERSION_MINOR > 10

#    define VERSION_MINOR_INIT        \
        ((VERSION_MINOR / 10) + '0'), \
            ((VERSION_MINOR % 10) + '0')

#else

#    define VERSION_MINOR_INIT \
        (VERSION_MINOR + '0')

#endif

#if VERSION_PATCH > 100

#    define VERSION_PATCH_INIT                    \
        ((VERSION_PATCH / 100) + '0'),            \
            (((VERSION_PATCH % 100) / 10) + '0'), \
            ((VERSION_PATCH % 10) + '0')

#elif VERSION_PATCH > 10

#    define VERSION_PATCH_INIT        \
        ((VERSION_PATCH / 10) + '0'), \
            ((VERSION_PATCH % 10) + '0')

#else

#    define VERSION_PATCH_INIT \
        (VERSION_PATCH + '0')

#endif

#ifndef HYPERDBG_KERNEL_MODE

const unsigned char BuildDateTime[] =
    {
        BUILD_YEAR_CH0,
        BUILD_YEAR_CH1,
        BUILD_YEAR_CH2,
        BUILD_YEAR_CH3,
        '-',
        BUILD_MONTH_CH0,
        BUILD_MONTH_CH1,
        '-',
        BUILD_DAY_CH0,
        BUILD_DAY_CH1,
        ' ',
        BUILD_HOUR_CH0,
        BUILD_HOUR_CH1,
        ':',
        BUILD_MIN_CH0,
        BUILD_MIN_CH1,
        ':',
        BUILD_SEC_CH0,
        BUILD_SEC_CH1,

        '\0'};

const unsigned char CompleteVersion[] =
    {
        'v',
        VERSION_MAJOR_INIT,
        '.',
        VERSION_MINOR_INIT,
        '.',
        VERSION_PATCH_INIT,
        '\0'};

const unsigned char BuildVersion[] =
    {
        BUILD_YEAR_CH0,
        BUILD_YEAR_CH1,
        BUILD_YEAR_CH2,
        BUILD_YEAR_CH3,
        BUILD_MONTH_CH0,
        BUILD_MONTH_CH1,
        BUILD_DAY_CH0,
        BUILD_DAY_CH1,

        '\0'};

#endif // SCRIPT_ENGINE_KERNEL_MODE

//////////////////////////////////////////////////
//				Message Tracing                 //
//////////////////////////////////////////////////

/**
 * @brief Default buffer count of packets for message tracing
 * @details number of packets storage for regualr buffers
 */
#define MaximumPacketsCapacity 1000

/**
 * @brief Default buffer count of packets for message tracing
 * @details number of packets storage for priority buffers
 */
#define MaximumPacketsCapacityPriority 10

/**
 * @brief Size of each packet
 * @details NOTE : REMEMBER TO CHANGE IT IN USER-MODE APP TOO
 * @warning we redefine it on ScriptEngineEval.h change it on
 * that file too
 */
#define PacketChunkSize 4096 // PAGE_SIZE

/**
 * @brief size of user-mode buffer
 * @details Because of Opeation code at the start of the
 * buffer + 1 for null-termminating
 *
 */
#define UsermodeBufferSize sizeof(UINT32) + PacketChunkSize + 1

/**
 * @brief size of buffer for serial
 * @details the maximum packet size for sending over serial
 * User-mode buffer size + Header Structure Size + Count Of End Buffer Bytes
 *
 */
#define MaxSerialPacketSize                               \
    UsermodeBufferSize + sizeof(DEBUGGER_REMOTE_PACKET) + \
        SERIAL_END_OF_BUFFER_CHARS_COUNT

/**
 * @brief Final storage size of message tracing
 *
 */
#define LogBufferSize \
    MaximumPacketsCapacity *(PacketChunkSize + sizeof(BUFFER_HEADER))

/**
 * @brief Final storage size of message tracing
 *
 */
#define LogBufferSizePriority \
    MaximumPacketsCapacityPriority *(PacketChunkSize + sizeof(BUFFER_HEADER))

/**
 * @brief limitation of Windows DbgPrint message size
 * @details currently is not functional
 *
 */
#define DbgPrintLimitation 512

/**
 * @brief The seeds that user-mode codes use as the starter
 * of their events' tag
 *
 */
#define DebuggerEventTagStartSeed 0x1000000

/**
 * @brief The seeds that user-mode thread detail token start with it
 * @details This seed should not start with zero (0), otherwise it's
 * interpreted as error
 */
#define DebuggerThreadDebuggingTagStartSeed 0x1000000

/**
 * @brief The seeds that user-mode codes use as the starter
 * of their output source tag
 *
 */
#define DebuggerOutputSourceTagStartSeed 0x1

/**
 * @brief Determines how many sources a debugger can have for
 * a single event
 *
 */
#define DebuggerOutputSourceMaximumRemoteSourceForSingleEvent 0x5

//////////////////////////////////////////////////
//               Remote Connection              //
//////////////////////////////////////////////////

/**
 * @brief default port of HyperDbg for listening by
 * debuggee (server, guest)
 *
 */
#define DEFAULT_PORT "50000"

/**
 * @brief Packet size for TCP connections
 * @details Note that we might add something to the kernel buffers
 * that's why we add 0x100 to it
 */
#define COMMUNICATION_BUFFER_SIZE PacketChunkSize + 0x100

//////////////////////////////////////////////////
//             Operation Codes                  //
//////////////////////////////////////////////////

/**
 * @brief If a operation use this bit in its Operation code,
 * then it means that the operation should be performed
 * mandatorily in debuggee and should not be sent to the debugger
 */
#define OPERATION_MANDATORY_DEBUGGEE_BIT (1 << 31)

/**
 * @brief Message logs id that comes from kernel-mode to
 * user-mode
 * @details Message area >= 0x5
 */
#define OPERATION_LOG_INFO_MESSAGE          0x1
#define OPERATION_LOG_WARNING_MESSAGE       0x2
#define OPERATION_LOG_ERROR_MESSAGE         0x3
#define OPERATION_LOG_NON_IMMEDIATE_MESSAGE 0x4
#define OPERATION_LOG_WITH_TAG              0x5

#define OPERATION_COMMAND_FROM_DEBUGGER_CLOSE_AND_UNLOAD_VMM \
    0x6 | OPERATION_MANDATORY_DEBUGGEE_BIT
#define OPERATION_DEBUGGEE_USER_INPUT     0x7 | OPERATION_MANDATORY_DEBUGGEE_BIT
#define OPERATION_DEBUGGEE_REGISTER_EVENT 0x8 | OPERATION_MANDATORY_DEBUGGEE_BIT
#define OPERATION_DEBUGGEE_ADD_ACTION_TO_EVENT \
    0x9 | OPERATION_MANDATORY_DEBUGGEE_BIT
#define OPERATION_DEBUGGEE_CLEAR_EVENTS 0xa | OPERATION_MANDATORY_DEBUGGEE_BIT
#define OPERATION_HYPERVISOR_DRIVER_IS_SUCCESSFULLY_LOADED \
    0xb | OPERATION_MANDATORY_DEBUGGEE_BIT
#define OPERATION_HYPERVISOR_DRIVER_END_OF_IRPS \
    0xc | OPERATION_MANDATORY_DEBUGGEE_BIT
#define OPERATION_COMMAND_FROM_DEBUGGER_RELOAD_SYMBOL \
    0xd | OPERATION_MANDATORY_DEBUGGEE_BIT

#define OPERATION_NOTIFICATION_FROM_USER_DEBUGGER_PAUSE \
    0xe | OPERATION_MANDATORY_DEBUGGEE_BIT

//////////////////////////////////////////////////
//            Breakpoint Backup                 //
//////////////////////////////////////////////////

/**
 * @brief maximum number of buffers to be allocated for a single
 * breakpoint
 */
#define MAXIMUM_BREAKPOINTS_WITHOUT_CONTINUE 50

//////////////////////////////////////////////////
//            End of Buffer Detection           //
//////////////////////////////////////////////////

/**
 * @brief count of characters for serial end of buffer
 */
#define SERIAL_END_OF_BUFFER_CHARS_COUNT 0x4

/**
 * @brief characters of the buffer that we set at the end of
 * buffers for serial
 */
#define SERIAL_END_OF_BUFFER_CHAR_1 0x00
#define SERIAL_END_OF_BUFFER_CHAR_2 0x80
#define SERIAL_END_OF_BUFFER_CHAR_3 0xEE
#define SERIAL_END_OF_BUFFER_CHAR_4 0xFF

/**
 * @brief count of characters for tcp end of buffer
 */
#define TCP_END_OF_BUFFER_CHARS_COUNT 0x4

/**
 * @brief characters of the buffer that we set at the end of
 * buffers for tcp
 */
#define TCP_END_OF_BUFFER_CHAR_1 0x10
#define TCP_END_OF_BUFFER_CHAR_2 0x20
#define TCP_END_OF_BUFFER_CHAR_3 0x33
#define TCP_END_OF_BUFFER_CHAR_4 0x44
