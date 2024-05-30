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
        UINT64 Inc : 1;
        UINT64 Dec : 1;
        UINT64 Or : 1;
        UINT64 Xor : 1;
        UINT64 And : 1;
        UINT64 Asr : 1;
        UINT64 Asl : 1;
        UINT64 Add : 1;
        UINT64 Sub : 1;
        UINT64 Mul : 1;
        UINT64 Div : 1;
        UINT64 Mod : 1;
        UINT64 Gt : 1;
        UINT64 Lt : 1;
        UINT64 Egt : 1;
        UINT64 Elt : 1;
        UINT64 Equal : 1;
        UINT64 Neq : 1;
        UINT64 Jmp : 1;
        UINT64 Jz : 1;
        UINT64 Jnz : 1;
        UINT64 Mov : 1;
        UINT64 Printf : 1;

    } scriptCapabilities;

    //
    // Here the details of port arrangements are located (HWDBG_PORT_INFORMATION_ITEMS)
    //

} HWDBG_SCRIPT_CAPABILITIES_INFORMATION, *PHWDBG_SCRIPT_CAPABILITIES_INFORMATION;
