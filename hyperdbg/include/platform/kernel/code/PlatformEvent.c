/**
 * @file PlatformEvent.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of cross platform APIs for kernel event and object management
 * @details
 * @version 0.19
 * @date 2026-05-09
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#if defined(__linux__)
#    include "../header/PlatformEvent.h"
#endif // defined(__linux__)

/**
 * @brief Dereference a kernel object, decrementing its reference count
 *
 * @param Object Pointer to the kernel object to dereference
 * @return VOID
 */
VOID
PlatformObjectDereference(PVOID Object)
{
#if defined(_WIN32) || defined(_WIN64)

    ObDereferenceObject(Object);

#elif defined(__linux__)

#    error "Not yet implemented"

#else

#    error "Unsupported platform"

#endif
}

/**
 * @brief Signal (set) a kernel event object
 *
 * @param Event Pointer to the KEVENT to signal
 * @param Increment Priority increment for any waiting threads to be awakened
 * @param Wait If TRUE, the caller intends to immediately call a wait routine after this call
 * @return LONG The previous signal state of the event
 */
LONG
PlatformEventSet(PKEVENT Event, KPRIORITY Increment, BOOLEAN Wait)
{
#if defined(_WIN32) || defined(_WIN64)

    return KeSetEvent(Event, Increment, Wait);

#elif defined(__linux__)

#    error "Not yet implemented"

#else

#    error "Unsupported platform"

#endif
}

/**
 * @brief Obtain a pointer to a kernel object by its user-mode handle and increment its reference count
 *
 * @param Handle User-mode handle referencing the kernel object
 * @param DesiredAccess Access mask for the requested access rights
 * @param ObjectType Pointer to the object type object (e.g., *ExEventObjectType); NULL to skip type check
 * @param AccessMode Processor mode to use for access checks (KernelMode or UserMode)
 * @param Object Receives a pointer to the referenced kernel object body
 * @param HandleInformation Optional; receives access state information
 * @return NTSTATUS STATUS_SUCCESS on success, or an error code on failure
 */
NTSTATUS
PlatformObjectReferenceByHandle(HANDLE                     Handle,
                                ACCESS_MASK                DesiredAccess,
                                POBJECT_TYPE               ObjectType,
                                KPROCESSOR_MODE            AccessMode,
                                PVOID *                    Object,
                                POBJECT_HANDLE_INFORMATION HandleInformation)
{
#if defined(_WIN32) || defined(_WIN64)

    return ObReferenceObjectByHandle(Handle,
                                     DesiredAccess,
                                     ObjectType,
                                     AccessMode,
                                     Object,
                                     HandleInformation);

#elif defined(__linux__)

#    error "Not yet implemented"

#else

#    error "Unsupported platform"

#endif
}
