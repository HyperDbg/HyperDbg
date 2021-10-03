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

/**
 * @brief maximum number of buffers to be allocated for a single
 * breakpoint
 */
#define MAXIMUM_POOLS_TO_PRE_ALLOCATE_ 50

//////////////////////////////////////////////////
//                   Enums                      //
//////////////////////////////////////////////////

/**
 * @brief Different resources handled in resource controller
 * 
 */
typedef enum _RESOURCE_CONTROLLER_TYPE
{
    //
    // Exception Bitmap Bits
    //
    RESOURCE_CONTROLER_TYPE_INVALD_INDEX           = 0,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_0 = 1,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_1,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_2,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_3,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_4,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_5,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_6,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_7,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_8,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_9,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_10,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_11,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_12,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_13,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_14,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_15,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_16,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_17,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_18,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_19,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_20,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_21,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_22,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_23,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_24,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_25,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_26,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_27,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_28,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_29,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_30,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_31,
    RESOURCE_CONTROLER_TYPE_EXCEPTION_BITMAP_BIT_32,

    //
    // Primary and Secondary based Control bits
    //
    RESOURCE_CONTROLER_TYPE_TSC_EXITING,
    RESOURCE_CONTROLER_TYPE_PMC_EXITING,
    RESOURCE_CONTROLER_EXTERNAL_INTERRUPT_EXITING,

    //
    // THIS SHOULD BE THE LAST ELEMENT
    //
    RESOURCE_CONTROLER_DESCRIPTORS_LENGTH,

} RESOURCE_CONTROLLER_TYPE;

//////////////////////////////////////////////////
//                   Structs                    //
//////////////////////////////////////////////////

/**
 * @brief Structure to save the state of resources
 * @details each core has its own resource state
 * 
 */
typedef struct _RESOURCE_CONTROLLER_RESOURCE_STATE
{
    RESOURCE_CONTROLLER_TYPE Type;            // Use as an indication of whether the resource was accessed or not
    UINT64                   RefCount;        // References to this resource count
    BOOLEAN                  IsForcedToUnset; // Whether it's unset by force (or a high priority task)

} RESOURCE_CONTROLLER_RESOURCE_STATE, *PRESOURCE_CONTROLLER_RESOURCE_STATE;

//////////////////////////////////////////////////
//                  Functions                   //
//////////////////////////////////////////////////

BOOLEAN
ResourceControllerInitialize(UINT32 TargetCore);

VOID
ResourceControllerUninitialize(UINT32 TargetCore);
