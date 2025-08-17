/**
 * @file CompatibilityChecks.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Header for checks for processor compatibility with different features
 * @details
 *
 * @version 0.2
 * @date 2023-03-15
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//////////////////////////////////////////////////
//				   Structures					//
//////////////////////////////////////////////////

/**
 * @brief The status of available features in the processor
 *
 */
typedef struct _COMPATIBILITY_CHECKS_STATUS
{
    BOOLEAN IsX2Apic;                  // X2APIC or XAPIC routine
    BOOLEAN RtmSupport;                // check for RTM support
    BOOLEAN PmlSupport;                // check Page Modification Logging (PML) support
    BOOLEAN ModeBasedExecutionSupport; // check for mode based execution support (processors after Kaby Lake release will support this feature)
    BOOLEAN ExecuteOnlySupport;        // Support for execute-only pages (indicating that data accesses are not allowed while instruction fetches are allowed)
    BOOLEAN CetIbtSupport;             // CET IBT support (indicating that indirect branch tracking is supported)
    BOOLEAN CetShadowStackSupport;     // CET shadow stack support (indicating that shadow stacks are supported)
    UINT32  VirtualAddressWidth;       // Virtual address width for x86 processors
    UINT32  PhysicalAddressWidth;      // Physical address width for x86 processors

} COMPATIBILITY_CHECKS_STATUS, *PCOMPATIBILITY_CHECKS_STATUS;

//////////////////////////////////////////////////
//				   Functions					//
//////////////////////////////////////////////////

VOID
CompatibilityCheckPerformChecks();
