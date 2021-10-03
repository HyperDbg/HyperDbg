/**
 * @file ResourceController.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Managing and keep the track of different resource
 * @details
 * @version 0.1
 * @date 2021-10-03
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "..\hprdbghv\pch.h"

/**
 * @brief Initialize the resource for the specific core
 * @details This function should be called in vmx non-root
 * @param TargetCore Target core index
 * 
 * @return BOOLEAN Shows whether the initialization was successful or not
 */
BOOLEAN
ResourceControllerInitialize(UINT32 TargetCore)
{
    //
    // Check to avoid re-allocation
    //
    if (g_GuestState[TargetCore].ResourceController != NULL)
    {
        //
        // Already allocated
        //
        return TRUE;
    }

    //
    // If we're here, it means that resource descriptors are not already allocated
    // and we should allocate it for this core
    //
    g_GuestState[TargetCore].ResourceController = ExAllocatePoolWithTag(NonPagedPool, RESOURCE_CONTROLER_DESCRIPTORS_LENGTH * sizeof(RESOURCE_CONTROLLER_RESOURCE_STATE), POOLTAG);

    if (g_GuestState[TargetCore].ResourceController == NULL)
    {
        //
        // Err, out of pool resource
        //
        return FALSE;
    }

    //
    // Zero the memory
    //
    RtlZeroMemory(g_GuestState[TargetCore].ResourceController, RESOURCE_CONTROLER_DESCRIPTORS_LENGTH * sizeof(RESOURCE_CONTROLLER_RESOURCE_STATE));

    //
    // Everything was okay, let's return to show it was successful
    //
    return TRUE;
}

/**
 * @brief Uninitialize the resource for the specific core
 * @details This function should be called in vmx non-root
 * @param TargetCore Target core index
 * 
 * @return VOID
 */
VOID
ResourceControllerUninitialize(UINT32 TargetCore)
{
    if (g_GuestState[TargetCore].ResourceController != NULL)
    {
        ExFreePoolWithTag(g_GuestState[TargetCore].ResourceController, POOLTAG);

        //
        // Set it to null to indicate that it's deallocated
        //
        g_GuestState[TargetCore].ResourceController = NULL;
    }
}

/**
 * @brief Allocates or find a resource descriptor struction
 *
 * @param CurrentCore current core Index
 * @param ResourceType The type of resource 
 * 
 * @return PRESOURCE_CONTROLLER_RESOURCE_STATE Returns the resource descriptor
 */
PRESOURCE_CONTROLLER_RESOURCE_STATE
ResourceControllerGetResourceDescriptor(UINT32                   CurrentCore,
                                        RESOURCE_CONTROLLER_TYPE ResourceType)
{
    PRESOURCE_CONTROLLER_RESOURCE_STATE ResourceState = NULL;

    //
    // Check to make sure that resource controller is already initialized
    // for this core
    //
    if (g_GuestState[CurrentCore].ResourceController == NULL)
    {
        //
        // Resource controller is not initialized for this core
        //
        return NULL;
    }

    //
    // Check if the ResourceType is valid
    //
    if (ResourceType <= RESOURCE_CONTROLER_TYPE_INVALD_INDEX || ResourceType >= RESOURCE_CONTROLER_DESCRIPTORS_LENGTH)
    {
        //
        // ResourceType is invalid
        //
        return NULL;
    }

    //
    // Get the resource
    //
    ResourceState = &g_GuestState[CurrentCore].ResourceController[ResourceType];

    //
    // Set the type of the resource, it's not needed but we use it as a
    // sign that the corresponding resource is already accessed
    //
    ResourceState->Type = ResourceType;

    return ResourceState;
}

/**
 * @brief Keep the track and applies a special resource
 * 
 * @param CurrentCore current core Index
 * @param ResourceType The type of resource 
 * @param IsSetAction Whether we should set or unset this resource 
 * @param IsForcedAction Is it a hight priority (forced) task or not 
 * 
 * @return BOOLEAN Returns whether the resource allocated successfully or not
 */
BOOLEAN
ResourceControllerGet(UINT32                   CurrentCore,
                      RESOURCE_CONTROLLER_TYPE ResourceType,
                      BOOLEAN                  IsSetAction,
                      BOOLEAN                  IsForcedAction)
{
    PRESOURCE_CONTROLLER_RESOURCE_STATE CurrentResourceState = NULL;

    //
    // Get the resource descriptor
    //
    CurrentResourceState = ResourceControllerGetResourceDescriptor(CurrentCore, ResourceType);

    if (CurrentResourceState == NULL)
    {
        //
        // Probably, ResourceType is invalid or resource controller
        // is not initialized
        //
        return FALSE;
    }

    //
    // Check if set a new reference or dereference
    //
    if (IsSetAction)
    {
        CurrentResourceState->RefCount = CurrentResourceState->RefCount + 1;
    }
    else
    {
        //
        // Avoid dereference a 0 RefCount, otherwise, we'll ignore it
        //
        if (CurrentResourceState->RefCount != 0)
        {
            CurrentResourceState->RefCount = CurrentResourceState->RefCount - 1;
        }
    }
}
