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
 * @brief Applies the effect of getting a resource
 * 
 * @param ResourceType The type of resource 
 * @param ResourceIndex Some resources contain index, otherwise null
 * @param TargetCore The target core(s) to apply 
 * @param CurrentCore The current core that tries to apply
 * @param IsSet Is it a set or unset action 
 * 
 * @return BOOLEAN Shows whether the operation was successful or not
 */
BOOLEAN
ResourceControllerApply(RESOURCE_CONTROLLER_TYPE ResourceType,
                        UINT64                   ResourceIndex,
                        UINT32                   TargetCore,
                        UINT32                   CurrentCore,
                        BOOLEAN                  IsSet)
{
    BOOLEAN Result                = FALSE;
    BOOLEAN IsBroadcastToAllCores = FALSE;

    //
    // Check if it's a broadcast to all cores or not
    //
    if (TargetCore == RESOURCE_CONTROLLER_APPLY_TO_ALL_CORES)
    {
        IsBroadcastToAllCores = TRUE;
    }

    //
    // Currently, HyperDbg won't support to broadcast a resource
    // to all the cores in vmx-root mode
    //
    if (IsBroadcastToAllCores && g_GuestState[CurrentCore].IsOnVmxRootMode)
    {
        //
        // Return is always FALSE;
        //
        return FALSE;
    }

    //
    // Apply the action to the resource
    //
    switch (ResourceType)
    {
    case RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP:

        break;

    default:
        Result = FALSE;
        break;
    }

    return Result;
}

/**
 * @brief Keep the track and applies a special resource
 * 
 * @param ResourceType The type of resource 
 * @param ResourceIndex Some resources contain index, otherwise null
 * @param Reason The reason to get this resource
 * @param TargetCore The target core(s) to apply 
 * @param IsSetAction Whether we should set or unset this resource 
 * @param IsHighPriorityTask Is it a hight priority (forced) task or not 
 * 
 * @return UINT64 Returns the tag that is needed to release the resource
 */
UINT64
ResourceControllerGet(RESOURCE_CONTROLLER_TYPE             ResourceType,
                      UINT64                               ResourceIndex,
                      RESOURCE_CONTROLLER_ASSIGNING_REASON Reason,
                      UINT32                               TargetCore,
                      BOOLEAN                              IsSetAction,
                      BOOLEAN                              IsHighPriorityTask)
{
    UINT32 CurrentCore;
    ULONG  ProcessorCount;

    CurrentCore    = KeGetCurrentProcessorNumber();
    ProcessorCount = KeQueryActiveProcessorCount(0);

    //
    // Check whether the target core is valid or not
    //
    if (TargetCore != RESOURCE_CONTROLLER_APPLY_TO_ALL_CORES)
    {
        //
        // Check if the core number is not invalid
        //
        if (TargetCore >= ProcessorCount)
        {
            return NULL;
        }
    }

    //
    // Add the resource to the list of resources
    //
}

/**
 * @brief Release a special tag
 * 
 * @param Tag The tag that was previously obtained from ResourceControllerGet 
 * 
 * @return BOOLEAN Returns the tag that is needed to release the resource
 */
BOOLEAN
ResourceControllerRelease(UINT64 Tag)
{
    //
    // Find the tag details
    //
}
