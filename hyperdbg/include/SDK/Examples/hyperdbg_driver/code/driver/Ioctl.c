/**
 * @file Ioctl.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief IOCTL Functions form user mode and other parts
 * @details
 *
 * @version 0.1
 * @date 2020-06-01
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Driver IOCTL Dispatcher
 *
 * @param DeviceObject
 * @param Irp
 * @return NTSTATUS
 */
NTSTATUS
DrvDispatchIoControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PIO_STACK_LOCATION      IrpStack;
    PREGISTER_NOTIFY_BUFFER RegisterEventRequest;
    NTSTATUS                Status;

    //
    // Here's the best place to see if there is any allocation pending
    // to be allcated as we're in PASSIVE_LEVEL
    //
    // DO NOT CHANGE CALLING OF THE FOLLOWING FUNCTION
    //
    PoolManagerCheckAndPerformAllocationAndDeallocation();

    if (g_AllowIOCTLFromUsermode)
    {
        IrpStack = IoGetCurrentIrpStackLocation(Irp);

        switch (IrpStack->Parameters.DeviceIoControl.IoControlCode)
        {
        case IOCTL_REGISTER_EVENT:

            //
            // First validate the parameters.
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < SIZEOF_REGISTER_EVENT || Irp->AssociatedIrp.SystemBuffer == NULL)
            {
                Status = STATUS_INVALID_PARAMETER;
                LogError("Err, invalid parameter to IOCTL dispatcher");
                break;
            }

            //
            // IRPs supply a pointer to a buffer at Irp->AssociatedIrp.SystemBuffer.
            // This buffer represents both the input buffer and the output buffer that
            // are specified in calls to DeviceIoControl
            //
            RegisterEventRequest = (PREGISTER_NOTIFY_BUFFER)Irp->AssociatedIrp.SystemBuffer;

            switch (RegisterEventRequest->Type)
            {
            case IRP_BASED:

                LogRegisterIrpBasedNotification((PVOID)Irp, &Status);

                break;
            case EVENT_BASED:

                if (LogRegisterEventBasedNotification((PVOID)Irp))
                {
                    Status = STATUS_SUCCESS;
                }
                else
                {
                    Status = STATUS_UNSUCCESSFUL;
                }

                break;
            default:
                LogError("Err, unknown notification type from user-mode");
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            break;

        default:
            LogError("Err, unknown IOCTL");
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        }
    }
    else
    {
        //
        // We're no longer serve IOCTL
        //
        Status = STATUS_SUCCESS;
    }

    if (Status != STATUS_PENDING)
    {
        Irp->IoStatus.Status = Status;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return Status;
}
