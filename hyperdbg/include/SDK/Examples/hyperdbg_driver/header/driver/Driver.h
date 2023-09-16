/**
 * @file Driver.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Header for WDK driver functions for RM
 * @details
 *
 * @version 0.2
 * @date 2023-01-29
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//			 WDK Major Functions				//
//////////////////////////////////////////////////

/**
 * @brief Load & Unload
 */
NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);

VOID
DrvUnload(PDRIVER_OBJECT DriverObject);

/**
 * @brief IRP Major Functions
 */
NTSTATUS
DrvCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS
DrvRead(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS
DrvWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS
DrvClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS
DrvUnsupported(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS
DrvDispatchIoControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);
