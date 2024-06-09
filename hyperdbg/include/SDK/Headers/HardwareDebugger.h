/**
 * @file HardwareDebugger.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief HyperDbg's Hardware Debugger (hwdbg) types and constants
 * @details This file contains definitions of hwdbg elements
 * used in HyperDbg
 * @version 0.9
 * @date 2024-04-28
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//                   Enums                      //
//////////////////////////////////////////////////

/**
 * @brief Different action of hwdbg
 * @warning This file should be changed along with hwdbg files
 *
 */
typedef enum _HWDBG_ACTION_ENUMS
{
    hwdbgActionSendVersion           = 1,
    hwdbgActionSendPinInformation    = 2,
    hwdbgActionConfigureScriptBuffer = 3,

} HWDBG_ACTION_ENUMS;

/**
 * @brief Different responses come from hwdbg
 * @warning This file should be changed along with hwdbg files
 *
 */
typedef enum _HWDBG_RESPONSE_ENUMS
{
    hwdbgResponseInvalidPacketOrError            = 1,
    hwdbgResponseVersion                         = 2,
    hwdbgResponsePinInformation                  = 3,
    hwdbgResponseScriptBufferConfigurationResult = 4,

} HWDBG_RESPONSE_ENUMS;

/**
 * @brief Different error codes in hwdbg
 * @warning This file should be changed along with hwdbg files
 *
 */
typedef enum _HWDBG_ERROR_ENUMS
{
    hwdbgErrorInvalidPacket = 1,

} HWDBG_ERROR_ENUMS;

//////////////////////////////////////////////////
//                   Structures                 //
//////////////////////////////////////////////////

/**
 * @brief The structure of port information (each item) in hwdbg
 *
 */
typedef struct _HWDBG_PORT_INFORMATION_ITEMS
{
    UINT32 PortSize;

} HWDBG_PORT_INFORMATION_ITEMS, *PHWDBG_PORT_INFORMATION_ITEMS;

/**
 * @brief The structure of script capabilities information in hwdbg
 *
 */
typedef struct _HWDBG_INSTANCE_INFORMATION
{
    UINT32 Version;                                 // Target version of HyperDbg (same as hwdbg)
    UINT32 MaximumNumberOfStages;                   // Number of stages that this instance of hwdbg supports (NumberOfSupportedStages == 0 means script engine is disabled)
    UINT32 scriptVariableLength;                    // maximum length of variables (and other script elements)
    UINT32 maximumNumberOfSupportedScriptOperators; // maximum supported operators in a single func
    UINT32 numberOfPins;                            // Number of pins
    UINT32 numberOfPorts;                           // Number of ports

    struct _HWDBG_SCRIPT_CAPABILITIES
    {
        UINT64 inc : 1;
        UINT64 dec : 1;
        UINT64 or : 1;
        UINT64 xor : 1;
        UINT64 and : 1;
        UINT64 asr : 1;
        UINT64 asl : 1;
        UINT64 add : 1;
        UINT64 sub : 1;
        UINT64 mul : 1;
        UINT64 div : 1;
        UINT64 mod : 1;
        UINT64 gt : 1;
        UINT64 lt : 1;
        UINT64 egt : 1;
        UINT64 elt : 1;
        UINT64 equal : 1;
        UINT64 neq : 1;
        UINT64 jmp : 1;
        UINT64 jz : 1;
        UINT64 jnz : 1;
        UINT64 mov : 1;
        UINT64 printf : 1;

    } scriptCapabilities;

    //
    // Here the details of port arrangements are located (HWDBG_PORT_INFORMATION_ITEMS)
    // As the following type:
    //   HWDBG_PORT_INFORMATION_ITEMS portsConfiguration[numberOfPorts]   ; Port arrangement
    //

} HWDBG_SCRIPT_CAPABILITIES_INFORMATION, *PHWDBG_SCRIPT_CAPABILITIES_INFORMATION;
