/**
 * @file PlatformEvent.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Cross platform APIs for kernel event and object management
 * @details
 * @version 0.19
 * @date 2026-05-09
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#if defined(__linux__)
#    include "../../../../include/SDK/HyperDbgSdk.h"
#endif // defined(__linux__)

#if defined(__linux__)

//
// NT's global event object-type pointer (ntddk). Shared code passes
// *ExEventObjectType when referencing a user-mode event handle; Linux has no
// object manager, so this is a placeholder token that the (fail-closed)
// PlatformObjectReferenceByHandle stub ignores. Defined in PlatformEvent.c.
//
extern POBJECT_TYPE * ExEventObjectType;

#endif // defined(__linux__)

//////////////////////////////////////////////////
//                  Functions                   //
//////////////////////////////////////////////////

VOID
PlatformObjectDereference(PVOID Object);

LONG
PlatformEventSet(PKEVENT Event, KPRIORITY Increment, BOOLEAN Wait);

NTSTATUS
PlatformObjectReferenceByHandle(HANDLE                    Handle,
                                ACCESS_MASK               DesiredAccess,
                                POBJECT_TYPE              ObjectType,
                                KPROCESSOR_MODE           AccessMode,
                                PVOID *                   Object,
                                POBJECT_HANDLE_INFORMATION HandleInformation);
