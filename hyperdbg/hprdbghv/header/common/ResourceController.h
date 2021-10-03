/**
 * @file ResourceController.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Header files for Managing and keep the track of different resource
 * @details
 * @version 0.1
 * @date 2021-10-03
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

//////////////////////////////////////////////////
//                   Constants                  //
//////////////////////////////////////////////////

#define RESOURCE_CONTROLLER_APPLY_TO_ALL_CORES DEBUGGER_EVENT_APPLY_TO_ALL_CORES

//////////////////////////////////////////////////
//                   Enums                      //
//////////////////////////////////////////////////

/**
 * @brief Different resources handled in resource controller
 * 
 */
typedef enum _RESOURCE_CONTROLLER_TYPE
{
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP = 1,
    RESOURCE_CONTROLER_TYPE_MSR_BITMAP,
    RESOURCE_CONTROLER_TYPE_IO_BITMAP,
    RESOURCE_CONTROLER_TYPE_TSC_EXITING,
    RESOURCE_CONTROLER_TYPE_PMC_EXITING,
    RESOURCE_CONTROLER_EXTERNAL_INTERRUPT_EXITING,

} RESOURCE_CONTROLLER_TYPE;

/**
 * @brief Different resources handled in resource controller
 * 
 */
typedef enum _RESOURCE_CONTROLLER_ASSIGNING_REASON
{
    RESOURCE_CONTROLLER_ASSIGNING_REASON_EXCEPTION_EVENT = 1,

} RESOURCE_CONTROLLER_ASSIGNING_REASON;

//////////////////////////////////////////////////
//                   Structs                    //
//////////////////////////////////////////////////

/**
 * @brief Structure to save the state of resources
 * 
 */
typedef struct _RESOURCE_CONTROLLER_RESOURCE_STATE
{
    UINT64                               Tag;
    RESOURCE_CONTROLLER_TYPE             Type;
    UINT64                               Index;
    RESOURCE_CONTROLLER_ASSIGNING_REASON Reason;
    UINT32                               TargetCore;
    BOOLEAN                              IsSetAction;
    BOOLEAN                              IsHighPriorityTask;

} RESOURCE_CONTROLLER_RESOURCE_STATE, *PRESOURCE_CONTROLLER_RESOURCE_STATE;

//////////////////////////////////////////////////
//                  Functions                   //
//////////////////////////////////////////////////

UINT64
ResourceControllerGet(RESOURCE_CONTROLLER_TYPE             ResourceType,
                      UINT64                               ResourceIndex,
                      RESOURCE_CONTROLLER_ASSIGNING_REASON Reason,
                      UINT32                               TargetCore,
                      BOOLEAN                              IsSetAction,
                      BOOLEAN                              IsHighPriorityTask);

BOOLEAN
ResourceControllerRelease(UINT64 Tag);
