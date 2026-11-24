/**
 * @file dbc.cpp
 * @author lava1879 (magicv4@proton.me)
 * @brief USB debugger transport through WinUSB
 * @details Reopens the device when Windows detects it again.
 * A reader thread keeps several reads pending and queues the bytes in USB order.
 * The debugger takes the bytes from that queue one at a time.
 * @version 0.1
 * @date 2026-11-18
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#ifdef _WIN32

//
// Make the notification types visible without changing the build target
//
#    pragma push_macro("WINVER")
#    undef WINVER
#    define WINVER _WIN32_WINNT_WIN8
#    include <cfgmgr32.h>
#    pragma pop_macro("WINVER")

#    include <setupapi.h>
#    include <winusb.h>

//
// Global Variables
//
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;
extern BOOLEAN g_SerialConnectionAlreadyClosed;

#    define DBC_USB_HARDWARE_ID_SUBSTRING L"vid_0525&pid_127a"
#    define DBC_USB_INTERFACE_CLASS       0xDC
#    define DBC_USB_INTERFACE_SUBCLASS    0x02

#    define DBC_USB_WRITE_TIMEOUT_MS    1000
#    define DBC_USB_OPEN_ERROR_DELAY_MS 3000

//
// Limits for pending USB reads and the receive queue
//
#    define DBC_USB_READ_COUNT             32
#    define DBC_USB_READ_QUEUE_SIZE        (512 * 1024)
#    define DBC_USB_READER_EXIT_TIMEOUT_MS 5000
#    define DBC_USB_STATISTICS_VARIABLE    "HYPERDBG_USB_STATS"
//
// Reserve space for all pending USB reads and one extra packet
//
#    define DBC_USB_READER_WAKE_BYTES ((DBC_USB_READ_COUNT + 1) * DBC_USB_MAX_PACKET_SIZE)
#    define DBC_QUEUE_EVENT_SPACE     1
#    define DBC_READ_EVENT_REMOVED    1
#    define DBC_READ_EVENT_DATA       2
#    define DBC_CONSUMER_EVENT_READER 2

//
// Standard USB device interface class GUID
//
static const GUID g_DbcUsbDeviceInterfaceGuid = {
    0xA5DCBF10,
    0x6530,
    0x11D2,
    {0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED}};

/**
 * @brief Open WinUSB device and its bulk pipes
 *
 */
typedef struct _DBC_USB_DEVICE
{
    volatile LONG           RefCount;
    volatile LONG           Failed;
    HANDLE                  DeviceHandle;
    WINUSB_INTERFACE_HANDLE WinUsbHandle;
    WCHAR *                 DevicePath;
    HANDLE                  RemovedEvent;
    HMODULE                 CfgMgr;
    HCMNOTIFICATION         RemovalNotification;
    UCHAR                   PipeIn;
    UCHAR                   PipeOut;
    USHORT                  PacketSizeIn;
    USHORT                  PacketSizeOut;

    decltype(&CM_Unregister_Notification) UnregisterNotification;

} DBC_USB_DEVICE, *PDBC_USB_DEVICE;

/**
 * @brief State of an active USB debugger connection
 *
 */
typedef struct _DBC_CONNECTION
{
    volatile LONG    RefCount;
    volatile LONG    Closed;
    HANDLE           CloseEvent;
    HANDLE           WriteEvent;
    HANDLE           ReaderThread;
    HANDLE           DataEvent;
    HANDLE           SpaceEvent;
    CRITICAL_SECTION DeviceLock;
    CRITICAL_SECTION OpenLock;
    CRITICAL_SECTION ReadLock;
    CRITICAL_SECTION WriteLock;
    CRITICAL_SECTION QueueLock;
    PDBC_USB_DEVICE  Device;
    UCHAR *          Queue;
    ULONG            QueueHead;
    ULONG            QueueLength;
    ULONG            QueuePeak;
    BOOLEAN          DeviceLost;
    BOOLEAN          ConsumerWaiting;
    BOOLEAN          ReaderWaiting;
    UINT64           BytesRead;
    UINT64           ReadsCompleted;
    UINT64           QueueFullWaits;
    DWORD            LastUsbOpenError;
    DWORD            UsbOpenErrorSince;
    BOOLEAN          UsbOpenErrorShown;
    BOOLEAN          UsbLayoutErrorShown;

} DBC_CONNECTION, *PDBC_CONNECTION;

//
// One pending read on the USB bulk IN pipe
//
typedef struct _DBC_USB_READ
{
    OVERLAPPED Overlapped;
    HANDLE     Event;
    UCHAR *    Buffer;
    BOOLEAN    Pending;

} DBC_USB_READ, *PDBC_USB_READ;

static volatile LONG   g_DbcActiveLock       = 0;
static PDBC_CONNECTION g_DbcActiveConnection = NULL;

/**
 * @brief Cancel I/O on a file handle from any thread
 * @details Look up CancelIoEx at run time to keep the Windows build target.
 *
 * @param FileHandle
 * @param Overlapped The transfer to cancel, or NULL for all transfers
 * @return VOID
 */
static VOID
DbcCancelFileIo(HANDLE FileHandle, LPOVERLAPPED Overlapped)
{
    typedef BOOL(WINAPI * CANCEL_IO_EX)(HANDLE, LPOVERLAPPED);

    static const CANCEL_IO_EX CancelIoExRoutine =
        (CANCEL_IO_EX)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "CancelIoEx");

    if (CancelIoExRoutine != NULL)
    {
        CancelIoExRoutine(FileHandle, Overlapped);
    }
}

/**
 * @brief Wake transfers when Windows removes the USB interface
 *
 * @param Notification
 * @param Context Device that registered for removal
 * @param Action
 * @param EventData
 * @param EventDataSize
 * @return DWORD
 */
static DWORD CALLBACK
DbcUsbInterfaceChanged(HCMNOTIFICATION       Notification,
                       PVOID                 Context,
                       CM_NOTIFY_ACTION      Action,
                       PCM_NOTIFY_EVENT_DATA EventData,
                       DWORD                 EventDataSize)
{
    UNREFERENCED_PARAMETER(Notification);
    UNREFERENCED_PARAMETER(EventDataSize);

    PDBC_USB_DEVICE Device = (PDBC_USB_DEVICE)Context;

    if (Action == CM_NOTIFY_ACTION_DEVICEINTERFACEREMOVAL &&
        _wcsicmp(Device->DevicePath, EventData->u.DeviceInterface.SymbolicLink) == 0)
    {
        SetEvent(Device->RemovedEvent);
    }

    return ERROR_SUCCESS;
}

/**
 * @brief Watch for removal before opening the WinUSB device
 *
 * @param Device
 * @param DevicePath
 * @return DWORD Windows error or ERROR_SUCCESS
 */
static DWORD
DbcWatchUsbRemoval(PDBC_USB_DEVICE Device, LPCWSTR DevicePath)
{
    CM_NOTIFY_FILTER                   Filter   = {0};
    decltype(&CM_Register_Notification) Register = NULL;

    Device->DevicePath = _wcsdup(DevicePath);

    if (Device->DevicePath == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Device->RemovedEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

    if (Device->RemovedEvent == NULL)
    {
        return GetLastError();
    }

    Device->CfgMgr = LoadLibraryW(L"cfgmgr32.dll");

    if (Device->CfgMgr == NULL)
    {
        return GetLastError();
    }

    Register                       = (decltype(Register))GetProcAddress(Device->CfgMgr, "CM_Register_Notification");
    Device->UnregisterNotification = (decltype(Device->UnregisterNotification))GetProcAddress(Device->CfgMgr, "CM_Unregister_Notification");

    if (Register == NULL || Device->UnregisterNotification == NULL)
    {
        return ERROR_SUCCESS;
    }

    Filter.cbSize                      = sizeof(Filter);
    Filter.FilterType                  = CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE;
    Filter.u.DeviceInterface.ClassGuid = g_DbcUsbDeviceInterfaceGuid;

    if (Register(&Filter, Device, DbcUsbInterfaceChanged, &Device->RemovalNotification) != CR_SUCCESS)
    {
        return ERROR_GEN_FAILURE;
    }

    return ERROR_SUCCESS;
}

/**
 * @brief Drop one reference to a WinUSB device and close handles at zero
 *
 * @param Device
 * @return VOID
 */
static VOID
DbcReleaseUsbDevice(PDBC_USB_DEVICE Device)
{
    if (Device == NULL)
    {
        return;
    }

    if (InterlockedDecrement(&Device->RefCount) == 0)
    {
        if (Device->RemovalNotification != NULL)
        {
            Device->UnregisterNotification(Device->RemovalNotification);
        }

        if (Device->RemovedEvent != NULL)
        {
            CloseHandle(Device->RemovedEvent);
        }

        if (Device->CfgMgr != NULL)
        {
            FreeLibrary(Device->CfgMgr);
        }

        if (Device->WinUsbHandle != NULL)
        {
            WinUsb_Free(Device->WinUsbHandle);
            Device->WinUsbHandle = NULL;
        }

        if (Device->DeviceHandle != NULL && Device->DeviceHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(Device->DeviceHandle);
            Device->DeviceHandle = INVALID_HANDLE_VALUE;
        }

        free(Device->DevicePath);
        free(Device);
    }
}

/**
 * @brief Drop one reference to a DbC connection and free resources at zero
 *
 * @param Connection
 * @return VOID
 */
static VOID
DbcReleaseConnection(PDBC_CONNECTION Connection)
{
    if (Connection == NULL)
    {
        return;
    }

    if (InterlockedDecrement(&Connection->RefCount) == 0)
    {
        if (Connection->Device != NULL)
        {
            DbcReleaseUsbDevice(Connection->Device);
            Connection->Device = NULL;
        }

        if (Connection->CloseEvent != NULL)
        {
            CloseHandle(Connection->CloseEvent);
            Connection->CloseEvent = NULL;
        }

        if (Connection->WriteEvent != NULL)
        {
            CloseHandle(Connection->WriteEvent);
        }

        if (Connection->ReaderThread != NULL)
        {
            CloseHandle(Connection->ReaderThread);
        }

        if (Connection->DataEvent != NULL)
        {
            CloseHandle(Connection->DataEvent);
        }

        if (Connection->SpaceEvent != NULL)
        {
            CloseHandle(Connection->SpaceEvent);
        }

        DeleteCriticalSection(&Connection->QueueLock);
        DeleteCriticalSection(&Connection->WriteLock);
        DeleteCriticalSection(&Connection->ReadLock);
        DeleteCriticalSection(&Connection->OpenLock);
        DeleteCriticalSection(&Connection->DeviceLock);

        free(Connection->Queue);
        free(Connection);
    }
}

/**
 * @brief Acquire a reference to the active DbC connection if Handle matches
 *
 * @param Handle
 * @return PDBC_CONNECTION
 */
static PDBC_CONNECTION
DbcAcquireConnection(HANDLE Handle)
{
    PDBC_CONNECTION Connection = NULL;

    if (Handle == NULL)
    {
        return NULL;
    }

    SpinlockLock(&g_DbcActiveLock);

    if (g_DbcActiveConnection != NULL &&
        (HANDLE)g_DbcActiveConnection == Handle &&
        g_DbcActiveConnection->Closed == 0)
    {
        InterlockedIncrement(&g_DbcActiveConnection->RefCount);
        Connection = g_DbcActiveConnection;
    }

    SpinlockUnlock(&g_DbcActiveLock);

    return Connection;
}

/**
 * @brief Find the bulk endpoints in the DbC interface
 *
 * @param WinUsbHandle
 * @param PipeIn Receives the bulk IN pipe information
 * @param PipeOut Receives the bulk OUT pipe information
 * @return BOOLEAN
 */
static BOOLEAN
DbcQueryUsbPipes(WINUSB_INTERFACE_HANDLE WinUsbHandle,
                 WINUSB_PIPE_INFORMATION * PipeIn,
                 WINUSB_PIPE_INFORMATION * PipeOut)
{
    USB_INTERFACE_DESCRIPTOR InterfaceDescriptor = {0};
    WINUSB_PIPE_INFORMATION  PipeInfo            = {0};

    ZeroMemory(PipeIn, sizeof(*PipeIn));
    ZeroMemory(PipeOut, sizeof(*PipeOut));

    if (!WinUsb_QueryInterfaceSettings(WinUsbHandle, 0, &InterfaceDescriptor) ||
        InterfaceDescriptor.bInterfaceNumber != 0 ||
        InterfaceDescriptor.bNumEndpoints != 2 ||
        InterfaceDescriptor.bInterfaceClass != DBC_USB_INTERFACE_CLASS ||
        InterfaceDescriptor.bInterfaceSubClass != DBC_USB_INTERFACE_SUBCLASS)
    {
        return FALSE;
    }

    for (UCHAR Index = 0; Index < InterfaceDescriptor.bNumEndpoints; Index++)
    {
        if (!WinUsb_QueryPipe(WinUsbHandle, 0, Index, &PipeInfo) ||
            PipeInfo.PipeType != UsbdPipeTypeBulk ||
            (PipeInfo.PipeId & 0x0F) == 0 ||
            PipeInfo.MaximumPacketSize == 0 ||
            PipeInfo.MaximumPacketSize > DBC_USB_MAX_PACKET_SIZE)
        {
            return FALSE;
        }

        if (USB_ENDPOINT_DIRECTION_IN(PipeInfo.PipeId) != 0)
        {
            if (PipeIn->PipeId != 0)
            {
                return FALSE;
            }

            *PipeIn = PipeInfo;
        }
        else
        {
            if (PipeOut->PipeId != 0)
            {
                return FALSE;
            }

            *PipeOut = PipeInfo;
        }
    }

    return PipeIn->PipeId != 0 && PipeOut->PipeId != 0;
}

/**
 * @brief Open a WinUSB device and set up its bulk pipes
 *
 * @param DevicePath
 * @param OpenError Receives the Windows error
 * @param UnsupportedLayout Set to TRUE if the endpoints do not match
 * @return PDBC_USB_DEVICE
 */
static PDBC_USB_DEVICE
DbcOpenUsbDeviceByPath(LPCWSTR DevicePath, DWORD * OpenError, BOOLEAN * UnsupportedLayout)
{
    PDBC_USB_DEVICE         Device               = NULL;
    WINUSB_PIPE_INFORMATION PipeIn               = {0};
    WINUSB_PIPE_INFORMATION PipeOut              = {0};
    ULONG                   ReadTimeoutMs        = 0;
    ULONG                   WriteTimeoutMs       = DBC_USB_WRITE_TIMEOUT_MS;
    UCHAR                   AutoClearStall       = 0;
    UCHAR                   IgnoreShortPackets   = 0;
    UCHAR                   ShortPacketTerminate = 0;
    UCHAR                   RawIo                = 1;

    Device = (PDBC_USB_DEVICE)calloc(1, sizeof(DBC_USB_DEVICE));

    if (Device == NULL)
    {
        *OpenError = ERROR_NOT_ENOUGH_MEMORY;
        return NULL;
    }

    Device->RefCount = 1;
    *OpenError       = DbcWatchUsbRemoval(Device, DevicePath);

    if (*OpenError != ERROR_SUCCESS)
    {
        DbcReleaseUsbDevice(Device);
        return NULL;
    }

    Device->DeviceHandle = CreateFileW(DevicePath,
                                       GENERIC_READ | GENERIC_WRITE,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       NULL,
                                       OPEN_EXISTING,
                                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                                       NULL);

    if (Device->DeviceHandle == INVALID_HANDLE_VALUE)
    {
        *OpenError = GetLastError();
        DbcReleaseUsbDevice(Device);
        return NULL;
    }

    if (!WinUsb_Initialize(Device->DeviceHandle, &Device->WinUsbHandle))
    {
        *OpenError = GetLastError();
        DbcReleaseUsbDevice(Device);
        return NULL;
    }

    if (!DbcQueryUsbPipes(Device->WinUsbHandle, &PipeIn, &PipeOut))
    {
        *UnsupportedLayout = TRUE;
        DbcReleaseUsbDevice(Device);
        return NULL;
    }

    //
    // Keep reads pending while idle and configure bulk short-packet policies
    //
    if (!WinUsb_SetPipePolicy(Device->WinUsbHandle,
                              PipeIn.PipeId,
                              PIPE_TRANSFER_TIMEOUT,
                              sizeof(ReadTimeoutMs),
                              &ReadTimeoutMs) ||
        !WinUsb_SetPipePolicy(Device->WinUsbHandle,
                              PipeIn.PipeId,
                              AUTO_CLEAR_STALL,
                              sizeof(AutoClearStall),
                              &AutoClearStall) ||
        !WinUsb_SetPipePolicy(Device->WinUsbHandle,
                              PipeIn.PipeId,
                              IGNORE_SHORT_PACKETS,
                              sizeof(IgnoreShortPackets),
                              &IgnoreShortPackets) ||
        !WinUsb_SetPipePolicy(Device->WinUsbHandle,
                              PipeOut.PipeId,
                              PIPE_TRANSFER_TIMEOUT,
                              sizeof(WriteTimeoutMs),
                              &WriteTimeoutMs) ||
        !WinUsb_SetPipePolicy(Device->WinUsbHandle,
                              PipeOut.PipeId,
                              SHORT_PACKET_TERMINATE,
                              sizeof(ShortPacketTerminate),
                              &ShortPacketTerminate))
    {
        *OpenError = GetLastError();
        DbcReleaseUsbDevice(Device);
        return NULL;
    }

    //
    // Submit reads directly when WinUSB accepts RAW_IO
    //
    WinUsb_SetPipePolicy(Device->WinUsbHandle, PipeIn.PipeId, RAW_IO, sizeof(RawIo), &RawIo);

    if (WaitForSingleObject(Device->RemovedEvent, 0) == WAIT_OBJECT_0)
    {
        *OpenError = ERROR_DEVICE_NOT_CONNECTED;
        DbcReleaseUsbDevice(Device);
        return NULL;
    }

    Device->PipeIn        = PipeIn.PipeId;
    Device->PipeOut       = PipeOut.PipeId;
    Device->PacketSizeIn  = PipeIn.MaximumPacketSize;
    Device->PacketSizeOut = PipeOut.MaximumPacketSize;

    return Device;
}

/**
 * @brief Check whether the USB device lacks the WinUSB driver
 *
 * @param DeviceInfoSet
 * @param DeviceInfoData
 * @return BOOLEAN
 */
static BOOLEAN
DbcIsWinUsbDriverMissing(HDEVINFO DeviceInfoSet, SP_DEVINFO_DATA * DeviceInfoData)
{
    WCHAR Service[257] = {0};

    if (!SetupDiGetDeviceRegistryPropertyW(DeviceInfoSet,
                                           DeviceInfoData,
                                           SPDRP_SERVICE,
                                           NULL,
                                           (PBYTE)Service,
                                           sizeof(Service),
                                           NULL))
    {
        return GetLastError() == ERROR_INVALID_DATA;
    }

    return _wcsicmp(Service, L"WinUSB") != 0;
}

/**
 * @brief Find and open the USB debug device through SetupAPI
 *
 * @param OpenError Receives the last open error or ERROR_SUCCESS
 * @param MissingWinUsb Set to TRUE if the device does not list WinUSB
 * @param UnsupportedLayout Set to TRUE if the endpoints do not match
 * @return PDBC_USB_DEVICE
 */
static PDBC_USB_DEVICE
DbcFindAndOpenUsbDevice(DWORD * OpenError, BOOLEAN * MissingWinUsb, BOOLEAN * UnsupportedLayout)
{
    HDEVINFO                           DeviceInfoSet = INVALID_HANDLE_VALUE;
    SP_DEVICE_INTERFACE_DATA           InterfaceData = {0};
    SP_DEVINFO_DATA                    DeviceInfoData = {0};
    PSP_DEVICE_INTERFACE_DETAIL_DATA_W DetailData    = NULL;
    PDBC_USB_DEVICE                    Device        = NULL;
    DWORD                              RequiredSize  = 0;

    *OpenError         = ERROR_SUCCESS;
    *MissingWinUsb     = FALSE;
    *UnsupportedLayout = FALSE;

    DeviceInfoSet = SetupDiGetClassDevsW(&g_DbcUsbDeviceInterfaceGuid,
                                         NULL,
                                         NULL,
                                         DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    if (DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        return NULL;
    }

    InterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    for (DWORD MemberIndex = 0;
         SetupDiEnumDeviceInterfaces(DeviceInfoSet,
                                     NULL,
                                     &g_DbcUsbDeviceInterfaceGuid,
                                     MemberIndex,
                                     &InterfaceData);
         MemberIndex++)
    {
        RequiredSize = 0;
        SetupDiGetDeviceInterfaceDetailW(DeviceInfoSet,
                                         &InterfaceData,
                                         NULL,
                                         0,
                                         &RequiredSize,
                                         NULL);

        if (RequiredSize == 0)
        {
            continue;
        }

        DetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)malloc(RequiredSize);

        if (DetailData == NULL)
        {
            continue;
        }

        DetailData->cbSize    = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        if (SetupDiGetDeviceInterfaceDetailW(DeviceInfoSet,
                                             &InterfaceData,
                                             DetailData,
                                             RequiredSize,
                                             NULL,
                                             &DeviceInfoData))
        {
            if (StrStrIW(DetailData->DevicePath, DBC_USB_HARDWARE_ID_SUBSTRING) != NULL)
            {
                Device         = DbcOpenUsbDeviceByPath(DetailData->DevicePath, OpenError, UnsupportedLayout);
                *MissingWinUsb = *OpenError != ERROR_SUCCESS &&
                                DbcIsWinUsbDriverMissing(DeviceInfoSet, &DeviceInfoData);
            }
        }

        free(DetailData);
        DetailData = NULL;

        if (Device != NULL)
        {
            break;
        }
    }

    SetupDiDestroyDeviceInfoList(DeviceInfoSet);

    return Device;
}

/**
 * @brief Report the expected USB endpoint layout once per reopen
 *
 * @param Connection
 * @return VOID
 */
static VOID
DbcReportUsbLayoutError(PDBC_CONNECTION Connection)
{
    if (!Connection->UsbLayoutErrorShown)
    {
        ShowMessages("err, unsupported USB debug device layout.\n"
                     "Expected interface 0 (class 0xDC, subclass 0x02), one bulk IN endpoint, one bulk OUT endpoint, and packets up to 1024 bytes.\n");
        Connection->UsbLayoutErrorShown = TRUE;
    }
}

/**
 * @brief Report a USB disconnect or an open error before the session starts
 *
 * @param Connection
 * @param Error Last device open error or ERROR_SUCCESS
 * @param MissingWinUsb TRUE if the failed device does not list WinUSB
 * @return VOID
 */
static VOID
DbcReportUsbOpenError(PDBC_CONNECTION Connection, DWORD Error, BOOLEAN MissingWinUsb)
{
    const CHAR * ErrorName = "unknown Windows error";
    BOOLEAN      Transient = TRUE;
    DWORD        Now       = GetTickCount();

    if (Connection->LastUsbOpenError != Error)
    {
        Connection->LastUsbOpenError  = Error;
        Connection->UsbOpenErrorSince = Now;
    }

    if (Connection->UsbOpenErrorShown)
    {
        return;
    }

    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        ShowMessages("err, USB debug device disconnected. Waiting for it to reconnect.\n");
        Connection->UsbOpenErrorShown = TRUE;
        return;
    }

    if (Error == ERROR_SUCCESS)
    {
        return;
    }

    switch (Error)
    {
    case ERROR_BAD_COMMAND:
        ErrorName = "ERROR_BAD_COMMAND";
        break;
    case ERROR_NOT_READY:
        ErrorName = "ERROR_NOT_READY";
        break;
    case ERROR_DEVICE_NOT_CONNECTED:
        ErrorName = "ERROR_DEVICE_NOT_CONNECTED";
        break;
    case ERROR_GEN_FAILURE:
        ErrorName = "ERROR_GEN_FAILURE";
        break;
    case ERROR_FILE_NOT_FOUND:
        ErrorName = "ERROR_FILE_NOT_FOUND";
        break;
    case ERROR_OPERATION_ABORTED:
        ErrorName = "ERROR_OPERATION_ABORTED";
        break;
    case ERROR_DEVICE_REMOVED:
        ErrorName = "ERROR_DEVICE_REMOVED";
        break;
    case ERROR_DEV_NOT_EXIST:
        ErrorName = "ERROR_DEV_NOT_EXIST";
        break;
    case ERROR_NO_SUCH_DEVICE:
        ErrorName = "ERROR_NO_SUCH_DEVICE";
        break;
    case ERROR_ACCESS_DENIED:
        ErrorName = "ERROR_ACCESS_DENIED";
        Transient = FALSE;
        break;
    case ERROR_SHARING_VIOLATION:
        ErrorName = "ERROR_SHARING_VIOLATION";
        Transient = FALSE;
        break;
    case ERROR_INVALID_PARAMETER:
        ErrorName = "ERROR_INVALID_PARAMETER";
        Transient = FALSE;
        break;
    case ERROR_INVALID_HANDLE:
        ErrorName = "ERROR_INVALID_HANDLE";
        Transient = FALSE;
        break;
    case ERROR_NOT_ENOUGH_MEMORY:
        ErrorName = "ERROR_NOT_ENOUGH_MEMORY";
        Transient = FALSE;
        break;
    case ERROR_BAD_DEVICE:
        ErrorName = "ERROR_BAD_DEVICE";
        Transient = FALSE;
        break;
    default:
        Transient = FALSE;
        break;
    }

    if (Transient && Now - Connection->UsbOpenErrorSince < DBC_USB_OPEN_ERROR_DELAY_MS)
    {
        return;
    }

    if (MissingWinUsb)
    {
        ShowMessages("err, WinUSB is not set for the USB debug device: %s, 0x%lx.\n"
                     "Run hyperdbg-usb-install.ps1 from the HyperDbg folder.\n",
                     ErrorName,
                     Error);
    }
    else
    {
        ShowMessages("err, unable to open USB debug device 0525:127a: %s, 0x%lx\n", ErrorName, Error);
    }

    Connection->UsbOpenErrorShown = TRUE;
}

/**
 * @brief Cancel pending transfers on both bulk pipes
 *
 * @param Device
 * @return VOID
 */
static VOID
DbcAbortUsbPipes(PDBC_USB_DEVICE Device)
{
    BOOL ReadAborted  = WinUsb_AbortPipe(Device->WinUsbHandle, Device->PipeIn);
    BOOL WriteAborted = WinUsb_AbortPipe(Device->WinUsbHandle, Device->PipeOut);

    if (!ReadAborted || !WriteAborted)
    {
        DbcCancelFileIo(Device->DeviceHandle, NULL);
    }
}

/**
 * @brief Mark the device as failed so the next attempt opens it again
 *
 * @param Connection
 * @param Device
 * @return VOID
 */
static VOID
DbcFailUsbDevice(PDBC_CONNECTION Connection, PDBC_USB_DEVICE Device)
{
    PDBC_USB_DEVICE DetachedDevice = NULL;

    if (Device == NULL)
    {
        return;
    }

    EnterCriticalSection(&Connection->DeviceLock);

    if (Connection->Device == Device)
    {
        InterlockedExchange(&Device->Failed, 1);
        SetEvent(Device->RemovedEvent);
        DbcAbortUsbPipes(Device);

        DetachedDevice     = Connection->Device;
        Connection->Device = NULL;
    }

    LeaveCriticalSection(&Connection->DeviceLock);

    if (DetachedDevice != NULL)
    {
        DbcReleaseUsbDevice(DetachedDevice);
    }
}

/**
 * @brief Take the device lock for a closing writer within a time limit
 *
 * @param Connection
 * @param TimeoutMilliseconds
 * @return BOOLEAN
 */
static BOOLEAN
DbcEnterDeviceLockWithTimeout(PDBC_CONNECTION Connection, DWORD TimeoutMilliseconds)
{
    DWORD Start = GetTickCount();

    //
    // The reader holds the lock only for a moment, so try again until the time limit
    //
    while (!TryEnterCriticalSection(&Connection->DeviceLock))
    {
        if (GetTickCount() - Start >= TimeoutMilliseconds)
        {
            return FALSE;
        }

        Sleep(1);
    }

    return TRUE;
}

/**
 * @brief Return the current open WinUSB device, or wait and reopen it
 *
 * @param Connection
 * @param ForWrite TRUE if a writer requests the device
 * @return PDBC_USB_DEVICE
 */
static PDBC_USB_DEVICE
DbcAcquireUsbDevice(PDBC_CONNECTION Connection, BOOLEAN ForWrite)
{
    PDBC_USB_DEVICE Device            = NULL;
    PDBC_USB_DEVICE NewDevice         = NULL;
    DWORD           OpenError         = ERROR_SUCCESS;
    BOOLEAN         MissingWinUsb     = FALSE;
    BOOLEAN         UnsupportedLayout = FALSE;

    if (Connection->Closed != 0)
    {
        return NULL;
    }

    if (ForWrite && g_SerialConnectionAlreadyClosed)
    {
        if (!DbcEnterDeviceLockWithTimeout(Connection, DBC_USB_WRITE_TIMEOUT_MS))
        {
            return NULL;
        }
    }
    else
    {
        EnterCriticalSection(&Connection->DeviceLock);
    }

    if (Connection->Device != NULL && Connection->Device->Failed == 0)
    {
        InterlockedIncrement(&Connection->Device->RefCount);
        Device = Connection->Device;
    }

    LeaveCriticalSection(&Connection->DeviceLock);

    //
    // Writers use only the current device and do not wait for a new one
    // The reader waits and opens the device again after it comes back
    //
    if (Device != NULL || ForWrite)
    {
        return Device;
    }

    while (Connection->Closed == 0)
    {
        EnterCriticalSection(&Connection->OpenLock);

        if (Connection->Closed != 0)
        {
            LeaveCriticalSection(&Connection->OpenLock);
            break;
        }

        EnterCriticalSection(&Connection->DeviceLock);

        if (Connection->Device != NULL && Connection->Device->Failed == 0)
        {
            InterlockedIncrement(&Connection->Device->RefCount);
            Device = Connection->Device;
        }

        LeaveCriticalSection(&Connection->DeviceLock);

        if (Device != NULL)
        {
            LeaveCriticalSection(&Connection->OpenLock);
            break;
        }

        NewDevice = DbcFindAndOpenUsbDevice(&OpenError, &MissingWinUsb, &UnsupportedLayout);

        if (NewDevice != NULL)
        {
            Connection->LastUsbOpenError    = ERROR_SUCCESS;
            Connection->UsbOpenErrorShown  = FALSE;
            Connection->UsbLayoutErrorShown = FALSE;

            EnterCriticalSection(&Connection->DeviceLock);

            if (Connection->Closed != 0)
            {
                LeaveCriticalSection(&Connection->DeviceLock);
                LeaveCriticalSection(&Connection->OpenLock);
                DbcReleaseUsbDevice(NewDevice);
                NewDevice = NULL;
                break;
            }

            //
            // Keep one reference for the connection and one for the caller
            //
            InterlockedIncrement(&NewDevice->RefCount);
            Connection->Device = NewDevice;
            Device             = NewDevice;

            LeaveCriticalSection(&Connection->DeviceLock);
            LeaveCriticalSection(&Connection->OpenLock);
            break;
        }

        if (Connection->Closed == 0 && !g_SerialConnectionAlreadyClosed)
        {
            DbcReportUsbOpenError(Connection, OpenError, MissingWinUsb);

            if (UnsupportedLayout)
            {
                DbcReportUsbLayoutError(Connection);
            }
        }

        LeaveCriticalSection(&Connection->OpenLock);

        if (WaitForSingleObject(Connection->CloseEvent, DBC_USB_REOPEN_DELAY_MS) == WAIT_OBJECT_0)
        {
            break;
        }
    }

    return Device;
}

/**
 * @brief Check whether a transfer error allows a pipe reset
 *
 * @param Error Windows error from the transfer
 * @return BOOLEAN
 */
static BOOLEAN
DbcCanResetPipe(DWORD Error)
{
    switch (Error)
    {
    case ERROR_GEN_FAILURE:
    case ERROR_IO_DEVICE:
        return TRUE;
    default:
        return FALSE;
    }
}

/**
 * @brief Reset a pipe if the device is still open
 *
 * @param Connection
 * @param Device
 * @param Pipe
 * @return BOOLEAN
 */
static BOOLEAN
DbcTryResetPipe(PDBC_CONNECTION Connection, PDBC_USB_DEVICE Device, UCHAR Pipe)
{
    BOOLEAN Result = FALSE;

    EnterCriticalSection(&Connection->DeviceLock);

    if (Connection->Closed == 0 && Device->Failed == 0 &&
        WaitForSingleObject(Device->RemovedEvent, 0) != WAIT_OBJECT_0)
    {
        Result = WinUsb_ResetPipe(Device->WinUsbHandle, Pipe) ? TRUE : FALSE;
    }

    LeaveCriticalSection(&Connection->DeviceLock);

    return Result;
}

/**
 * @brief Wake the reader thread if it waits for the consumer
 * @details The caller holds the queue lock.
 *
 * @param Connection
 * @return VOID
 */
static VOID
DbcWakeQueueReader(PDBC_CONNECTION Connection)
{
    //
    // Wake the reader when the queue has room for its pending USB reads
    //
    if (Connection->ReaderWaiting && !Connection->DeviceLost &&
        DBC_USB_READ_QUEUE_SIZE - Connection->QueueLength >= DBC_USB_READER_WAKE_BYTES)
    {
        Connection->ReaderWaiting = FALSE;
        SetEvent(Connection->SpaceEvent);
    }
}

/**
 * @brief Wait until the consumer frees space in the queue or takes the device loss
 * @details The caller holds the queue lock, and this function releases it.
 *
 * @param Connection
 * @return BOOLEAN FALSE if the connection closes first
 */
static BOOLEAN
DbcWaitForQueueConsumer(PDBC_CONNECTION Connection)
{
    HANDLE Events[] = {Connection->CloseEvent, Connection->SpaceEvent};

    Connection->ReaderWaiting = TRUE;
    ResetEvent(Connection->SpaceEvent);

    LeaveCriticalSection(&Connection->QueueLock);

    return WaitForMultipleObjects(ARRAYSIZE(Events), Events, FALSE, INFINITE) == WAIT_OBJECT_0 + DBC_QUEUE_EVENT_SPACE;
}

/**
 * @brief Add received bytes to the end of the queue in USB order
 * @details Waits for the consumer while the queue is full.
 *
 * @param Connection
 * @param Data
 * @param Length
 * @return BOOLEAN FALSE if the connection closes while the queue is full
 */
static BOOLEAN
DbcQueueAppend(PDBC_CONNECTION Connection, const UCHAR * Data, ULONG Length)
{
    ULONG Free  = 0;
    ULONG Chunk = 0;
    ULONG Tail  = 0;
    ULONG First = 0;

    while (Length != 0)
    {
        EnterCriticalSection(&Connection->QueueLock);

        Free = DBC_USB_READ_QUEUE_SIZE - Connection->QueueLength;

        if (Free == 0)
        {
            Connection->QueueFullWaits++;

            if (!DbcWaitForQueueConsumer(Connection))
            {
                return FALSE;
            }

            continue;
        }

        Chunk = Length < Free ? Length : Free;
        Tail  = (Connection->QueueHead + Connection->QueueLength) % DBC_USB_READ_QUEUE_SIZE;
        First = DBC_USB_READ_QUEUE_SIZE - Tail;

        if (First > Chunk)
        {
            First = Chunk;
        }

        memcpy(Connection->Queue + Tail, Data, First);
        memcpy(Connection->Queue, Data + First, Chunk - First);

        Connection->QueueLength += Chunk;

        if (Connection->QueueLength > Connection->QueuePeak)
        {
            Connection->QueuePeak = Connection->QueueLength;
        }

        if (Connection->ConsumerWaiting)
        {
            Connection->ConsumerWaiting = FALSE;
            SetEvent(Connection->DataEvent);
        }

        LeaveCriticalSection(&Connection->QueueLock);

        Data += Chunk;
        Length -= Chunk;
    }

    return TRUE;
}

/**
 * @brief Tell the consumer that the device was lost after the queued bytes
 * @details Waits until the consumer takes the loss, so the bytes of the next device come after it.
 *
 * @param Connection
 * @return BOOLEAN FALSE if the connection closes first
 */
static BOOLEAN
DbcQueueDeviceLoss(PDBC_CONNECTION Connection)
{
    EnterCriticalSection(&Connection->QueueLock);

    Connection->DeviceLost = TRUE;

    if (Connection->ConsumerWaiting)
    {
        Connection->ConsumerWaiting = FALSE;
        SetEvent(Connection->DataEvent);
    }

    while (Connection->DeviceLost)
    {
        if (!DbcWaitForQueueConsumer(Connection))
        {
            return FALSE;
        }

        EnterCriticalSection(&Connection->QueueLock);
    }

    LeaveCriticalSection(&Connection->QueueLock);

    return TRUE;
}

/**
 * @brief Submit reads on the bulk IN pipe in queue order
 *
 * @param Connection
 * @param Device
 * @param Reads
 * @param Start First read to submit
 * @param Count Number of reads to submit
 * @return DWORD Windows error or ERROR_SUCCESS
 */
static DWORD
DbcSubmitUsbReads(PDBC_CONNECTION Connection,
                  PDBC_USB_DEVICE Device,
                  PDBC_USB_READ   Reads,
                  ULONG           Start,
                  ULONG           Count)
{
    PDBC_USB_READ Read        = NULL;
    ULONG         Transferred = 0;
    DWORD         Error       = ERROR_SUCCESS;

    //
    // Submit the reads before a device failure can abort the pipe
    //
    EnterCriticalSection(&Connection->DeviceLock);

    if (WaitForSingleObject(Device->RemovedEvent, 0) == WAIT_OBJECT_0)
    {
        Error = ERROR_DEVICE_NOT_CONNECTED;
    }
    else if (Connection->Closed != 0 || Device->Failed != 0)
    {
        Error = ERROR_OPERATION_ABORTED;
    }

    for (ULONG Index = 0; Index < Count && Error == ERROR_SUCCESS; Index++)
    {
        Read = &Reads[(Start + Index) % DBC_USB_READ_COUNT];

        PlatformZeroMemory(&Read->Overlapped, sizeof(OVERLAPPED));
        Read->Overlapped.hEvent = Read->Event;
        ResetEvent(Read->Event);

        if (!WinUsb_ReadPipe(Device->WinUsbHandle,
                             Device->PipeIn,
                             Read->Buffer,
                             Device->PacketSizeIn,
                             &Transferred,
                             &Read->Overlapped))
        {
            Error = GetLastError();

            if (Error != ERROR_IO_PENDING)
            {
                break;
            }

            Error = ERROR_SUCCESS;
        }

        Read->Pending = TRUE;
    }

    LeaveCriticalSection(&Connection->DeviceLock);

    return Error;
}

/**
 * @brief Cancel pending reads, collect every result, and queue successful data in order
 * @details A loss mark separates successful data across failed reads.
 *
 * @param Connection
 * @param Device
 * @param Reads
 * @param Head Oldest submitted read
 * @return BOOLEAN FALSE if the consumer stops taking the completed data
 */
static BOOLEAN
DbcCancelUsbReads(PDBC_CONNECTION Connection, PDBC_USB_DEVICE Device, PDBC_USB_READ Reads, ULONG Head)
{
    ULONG   Transferred[DBC_USB_READ_COUNT] = {0};
    BOOLEAN Completed[DBC_USB_READ_COUNT]   = {0};
    BOOLEAN Succeeded[DBC_USB_READ_COUNT]   = {0};
    BOOLEAN AnyPending                     = FALSE;
    BOOLEAN Lost                           = FALSE;
    BOOLEAN Result                         = TRUE;

    for (ULONG Index = 0; Index < DBC_USB_READ_COUNT; Index++)
    {
        AnyPending |= Reads[Index].Pending;
    }

    if (!AnyPending)
    {
        return TRUE;
    }

    EnterCriticalSection(&Connection->DeviceLock);

    if (!WinUsb_AbortPipe(Device->WinUsbHandle, Device->PipeIn))
    {
        for (ULONG Index = 0; Index < DBC_USB_READ_COUNT; Index++)
        {
            if (Reads[Index].Pending)
            {
                DbcCancelFileIo(Device->DeviceHandle, &Reads[Index].Overlapped);
            }
        }
    }

    LeaveCriticalSection(&Connection->DeviceLock);

    //
    // Cancellation can finish normally, so collect every result before reusing buffers
    //
    for (ULONG Index = 0; Index < DBC_USB_READ_COUNT; Index++)
    {
        ULONG Slot = (Head + Index) % DBC_USB_READ_COUNT;

        if (Reads[Slot].Pending)
        {
            Completed[Slot] = TRUE;
            Succeeded[Slot] = WinUsb_GetOverlappedResult(Device->WinUsbHandle,
                                                         &Reads[Slot].Overlapped,
                                                         &Transferred[Slot],
                                                         TRUE) != FALSE;
            Reads[Slot].Pending = FALSE;
        }
    }

    for (ULONG Index = 0; Index < DBC_USB_READ_COUNT; Index++)
    {
        ULONG Slot = (Head + Index) % DBC_USB_READ_COUNT;

        if (!Completed[Slot])
        {
            continue;
        }

        if (!Succeeded[Slot])
        {
            Lost = TRUE;
            continue;
        }

        if (Connection->Closed != 0)
        {
            Result = FALSE;
        }

        if (Result && Lost)
        {
            Result = DbcQueueDeviceLoss(Connection);
            Lost   = FALSE;
        }

        if (Transferred[Slot] > Device->PacketSizeIn)
        {
            Transferred[Slot] = Device->PacketSizeIn;
        }

        if (Result)
        {
            Result = DbcQueueAppend(Connection, Reads[Slot].Buffer, Transferred[Slot]);
            if (Result)
            {
                Connection->BytesRead += Transferred[Slot];
                Connection->ReadsCompleted++;
            }
        }
    }

    return Result;
}

/**
 * @brief Keep reads pending on one device and queue their data in USB order
 *
 * @param Connection
 * @param Device
 * @param Reads
 * @return BOOLEAN TRUE if the device failed and the reader opens it again
 */
static BOOLEAN
DbcReadUsbDevice(PDBC_CONNECTION Connection, PDBC_USB_DEVICE Device, PDBC_USB_READ Reads)
{
    HANDLE  Events[]     = {Connection->CloseEvent, Device->RemovedEvent, NULL};
    UCHAR * Buffers      = NULL;
    ULONG   Head         = 0;
    ULONG   Start        = 0;
    ULONG   Completed    = 0;
    ULONG   Transferred  = 0;
    DWORD   WaitResult   = 0;
    DWORD   Error        = ERROR_SUCCESS;
    BOOLEAN PipeReset    = FALSE;
    BOOLEAN DeviceFailed = FALSE;

    Buffers = (UCHAR *)malloc((SIZE_T)Device->PacketSizeIn * DBC_USB_READ_COUNT);

    if (Buffers == NULL)
    {
        ShowMessages("err, not enough memory for the USB read buffers\n");
        return FALSE;
    }

    for (ULONG Index = 0; Index < DBC_USB_READ_COUNT; Index++)
    {
        Reads[Index].Buffer  = Buffers + (SIZE_T)Index * Device->PacketSizeIn;
        Reads[Index].Pending = FALSE;
    }

    Error = DbcSubmitUsbReads(Connection, Device, Reads, 0, DBC_USB_READ_COUNT);

    while (Connection->Closed == 0)
    {
        if (Error == ERROR_SUCCESS)
        {
            Events[DBC_READ_EVENT_DATA]  = Reads[Head].Event;
            WaitResult = WaitForMultipleObjects(ARRAYSIZE(Events), Events, FALSE, INFINITE);

            if (WaitResult == WAIT_OBJECT_0)
            {
                break;
            }
            else if (WaitResult == WAIT_OBJECT_0 + DBC_READ_EVENT_REMOVED)
            {
                Error = ERROR_DEVICE_NOT_CONNECTED;
            }
            else if (WaitResult != WAIT_OBJECT_0 + DBC_READ_EVENT_DATA)
            {
                Error = ERROR_GEN_FAILURE;
            }
        }

        //
        // Take completed USB reads in submission order
        //
        Start     = Head;
        Completed = 0;

        while (Error == ERROR_SUCCESS && Completed < DBC_USB_READ_COUNT &&
               (Completed == 0 || HasOverlappedIoCompleted(&Reads[Head].Overlapped)))
        {
            Transferred = 0;

            if (!WinUsb_GetOverlappedResult(Device->WinUsbHandle,
                                            &Reads[Head].Overlapped,
                                            &Transferred,
                                            Completed == 0))
            {
                Error = GetLastError();
                break;
            }

            Reads[Head].Pending = FALSE;
            PipeReset           = FALSE;

            if (Transferred > Device->PacketSizeIn)
            {
                Transferred = Device->PacketSizeIn;
            }

            if (!DbcQueueAppend(Connection, Reads[Head].Buffer, Transferred))
            {
                Error = ERROR_OPERATION_ABORTED;
                break;
            }

            Connection->BytesRead += Transferred;
            Connection->ReadsCompleted++;

            Head = (Head + 1) % DBC_USB_READ_COUNT;
            Completed++;
        }

        //
        // Submit the completed reads again behind the ones still pending
        //
        if (Error == ERROR_SUCCESS)
        {
            Error = DbcSubmitUsbReads(Connection, Device, Reads, Start, Completed);

            if (Error == ERROR_SUCCESS)
            {
                continue;
            }
        }

        //
        // Collect successful completions even when cancellation races with them
        //
        if (!DbcCancelUsbReads(Connection, Device, Reads, Head))
        {
            DeviceFailed = Connection->Closed == 0;
            break;
        }

        if (Connection->Closed != 0)
        {
            break;
        }

        if (!PipeReset && DbcCanResetPipe(Error))
        {
            if (!DbcQueueDeviceLoss(Connection))
            {
                break;
            }

            if (DbcTryResetPipe(Connection, Device, Device->PipeIn))
            {
                PipeReset = TRUE;
                Head      = 0;
                Error     = DbcSubmitUsbReads(Connection, Device, Reads, 0, DBC_USB_READ_COUNT);
                continue;
            }
        }

        DeviceFailed = TRUE;
        break;
    }

    if (!DbcCancelUsbReads(Connection, Device, Reads, Head))
    {
        DeviceFailed = Connection->Closed == 0;
    }

    if (DeviceFailed)
    {
        DbcFailUsbDevice(Connection, Device);
    }

    free(Buffers);

    return DeviceFailed;
}

/**
 * @brief Read from the USB device and open it again after each loss
 * @details One reader thread runs for each connection and owns all pending reads.
 *
 * @param Parameter Connection with a reference held for this thread
 * @return DWORD
 */
static DWORD WINAPI
DbcUsbReaderThread(LPVOID Parameter)
{
    PDBC_CONNECTION Connection                = (PDBC_CONNECTION)Parameter;
    PDBC_USB_DEVICE Device                    = NULL;
    DBC_USB_READ    Reads[DBC_USB_READ_COUNT] = {0};
    BOOLEAN         DeviceFailed              = FALSE;

    for (ULONG Index = 0; Index < DBC_USB_READ_COUNT; Index++)
    {
        Reads[Index].Event = CreateEventW(NULL, TRUE, FALSE, NULL);

        if (Reads[Index].Event == NULL)
        {
            ShowMessages("err, unable to create USB read events\n");
            goto Out;
        }
    }

    while (Connection->Closed == 0)
    {
        //
        // Wait for the device and open it again after a loss
        //
        Device = DbcAcquireUsbDevice(Connection, FALSE);

        if (Device == NULL)
        {
            break;
        }

        DeviceFailed = DbcReadUsbDevice(Connection, Device, Reads);

        DbcReleaseUsbDevice(Device);
        Device = NULL;

        if (!DeviceFailed || !DbcQueueDeviceLoss(Connection))
        {
            break;
        }
    }

Out:
    for (ULONG Index = 0; Index < DBC_USB_READ_COUNT; Index++)
    {
        if (Reads[Index].Event != NULL)
        {
            CloseHandle(Reads[Index].Event);
        }
    }

    DbcReleaseConnection(Connection);

    return 0;
}

/**
 * @brief Show the read statistics if the environment variable is set
 *
 * @param Connection
 * @return VOID
 */
static VOID
DbcShowReadStatistics(PDBC_CONNECTION Connection)
{
    if (GetEnvironmentVariableA(DBC_USB_STATISTICS_VARIABLE, NULL, 0) == 0)
    {
        return;
    }

    ShowMessages("received over USB: %llu bytes in %llu transfers, queue peak %lu bytes, %llu waits for queue space\n",
                 Connection->BytesRead,
                 Connection->ReadsCompleted,
                 Connection->QueuePeak,
                 Connection->QueueFullWaits);
}

/**
 * @brief Create a USB debugger connection
 *
 * @return HANDLE
 */
HANDLE
DbcConnect()
{
    PDBC_CONNECTION Connection        = NULL;
    PDBC_CONNECTION StaleConnection   = NULL;
    PDBC_USB_DEVICE InitialDevice     = NULL;
    DWORD           OpenError         = ERROR_SUCCESS;
    BOOLEAN         MissingWinUsb     = FALSE;
    BOOLEAN         UnsupportedLayout = FALSE;

    Connection = (PDBC_CONNECTION)malloc(sizeof(DBC_CONNECTION));

    if (Connection == NULL)
    {
        ShowMessages("err, not enough memory for USB debugging\n");
        return NULL;
    }

    PlatformZeroMemory(Connection, sizeof(DBC_CONNECTION));

    Connection->CloseEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

    if (Connection->CloseEvent == NULL)
    {
        free(Connection);
        ShowMessages("err, unable to create the USB close event\n");
        return NULL;
    }

    InitializeCriticalSection(&Connection->DeviceLock);
    InitializeCriticalSection(&Connection->OpenLock);
    InitializeCriticalSection(&Connection->ReadLock);
    InitializeCriticalSection(&Connection->WriteLock);
    InitializeCriticalSection(&Connection->QueueLock);

    Connection->RefCount = 1;

    Connection->WriteEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    Connection->DataEvent  = CreateEventW(NULL, TRUE, FALSE, NULL);
    Connection->SpaceEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

    if (Connection->WriteEvent == NULL || Connection->DataEvent == NULL || Connection->SpaceEvent == NULL)
    {
        DbcReleaseConnection(Connection);
        ShowMessages("err, unable to create USB transfer events\n");
        return NULL;
    }

    Connection->Queue = (UCHAR *)malloc(DBC_USB_READ_QUEUE_SIZE);

    if (Connection->Queue == NULL)
    {
        DbcReleaseConnection(Connection);
        ShowMessages("err, not enough memory for the USB read queue\n");
        return NULL;
    }

    //
    // If the device is absent, the reader thread waits for it
    //
    InitialDevice = DbcFindAndOpenUsbDevice(&OpenError, &MissingWinUsb, &UnsupportedLayout);

    if (InitialDevice != NULL)
    {
        Connection->Device = InitialDevice;
    }
    else if (UnsupportedLayout)
    {
        DbcReportUsbLayoutError(Connection);
        DbcReleaseConnection(Connection);
        return NULL;
    }
    else
    {
        DbcReportUsbOpenError(Connection, OpenError, MissingWinUsb);
    }

    //
    // The reader thread holds its own reference to the connection
    //
    InterlockedIncrement(&Connection->RefCount);

    Connection->ReaderThread = CreateThread(NULL, 0, DbcUsbReaderThread, Connection, 0, NULL);

    if (Connection->ReaderThread == NULL)
    {
        InterlockedDecrement(&Connection->RefCount);
        DbcReleaseConnection(Connection);
        ShowMessages("err, unable to create the USB reader thread\n");
        return NULL;
    }

    SpinlockLock(&g_DbcActiveLock);
    StaleConnection       = g_DbcActiveConnection;
    g_DbcActiveConnection = Connection;
    SpinlockUnlock(&g_DbcActiveLock);

    if (StaleConnection != NULL)
    {
        InterlockedExchange(&StaleConnection->Closed, 1);
        SetEvent(StaleConnection->CloseEvent);
        DbcReleaseConnection(StaleConnection);
    }

    return (HANDLE)Connection;
}

/**
 * @brief Submit a USB transfer and wait for it
 *
 * @param Connection
 * @param Device
 * @param Pipe
 * @param Buffer
 * @param Length
 * @param TransferEvent
 * @param WaitTimeout Maximum wait time in milliseconds, or INFINITE
 * @param Transferred Receives the transfer length
 * @return DWORD Windows error or ERROR_SUCCESS
 */
static DWORD
DbcTransferUsbPipe(PDBC_CONNECTION Connection,
                   PDBC_USB_DEVICE Device,
                   UCHAR           Pipe,
                   PUCHAR          Buffer,
                   ULONG           Length,
                   HANDLE          TransferEvent,
                   DWORD           WaitTimeout,
                   DWORD *         Transferred)
{
    OVERLAPPED Overlapped = {0};
    HANDLE     Events[]   = {Connection->CloseEvent, Device->RemovedEvent, TransferEvent};
    BOOL       Submitted  = FALSE;
    DWORD      Error      = ERROR_OPERATION_ABORTED;
    DWORD      WaitResult = WAIT_OBJECT_0 + 2;

    Overlapped.hEvent = TransferEvent;

    //
    // Submit the transfer before a device failure can abort its pipe
    //
    if (WaitTimeout == INFINITE)
    {
        EnterCriticalSection(&Connection->DeviceLock);
    }
    else if (!DbcEnterDeviceLockWithTimeout(Connection, WaitTimeout))
    {
        return ERROR_OPERATION_ABORTED;
    }

    if (WaitForSingleObject(Device->RemovedEvent, 0) == WAIT_OBJECT_0)
    {
        Error = ERROR_DEVICE_NOT_CONNECTED;
    }
    else if (Connection->Closed == 0 && Device->Failed == 0)
    {
        if (Pipe == Device->PipeIn)
        {
            Submitted = WinUsb_ReadPipe(Device->WinUsbHandle, Pipe, Buffer, Length, Transferred, &Overlapped);
        }
        else
        {
            Submitted = WinUsb_WritePipe(Device->WinUsbHandle, Pipe, Buffer, Length, Transferred, &Overlapped);
        }

        Error = Submitted ? ERROR_SUCCESS : GetLastError();
    }

    LeaveCriticalSection(&Connection->DeviceLock);

    if (Error != ERROR_SUCCESS && Error != ERROR_IO_PENDING)
    {
        return Error;
    }

    if (Error == ERROR_IO_PENDING)
    {
        WaitResult = WaitForMultipleObjects(ARRAYSIZE(Events), Events, FALSE, WaitTimeout);
    }

    if (WaitResult == WAIT_TIMEOUT || WaitResult == WAIT_OBJECT_0 + 1)
    {
        DbcCancelFileIo(Device->DeviceHandle, &Overlapped);
    }
    else if (WaitResult != WAIT_OBJECT_0 + 2)
    {
        EnterCriticalSection(&Connection->DeviceLock);

        if (!HasOverlappedIoCompleted(&Overlapped) &&
            !WinUsb_AbortPipe(Device->WinUsbHandle, Pipe))
        {
            DbcCancelFileIo(Device->DeviceHandle, &Overlapped);
        }

        LeaveCriticalSection(&Connection->DeviceLock);
    }

    //
    // Keep the buffer and OVERLAPPED alive until the transfer completes
    //
    Error = WinUsb_GetOverlappedResult(Device->WinUsbHandle, &Overlapped, Transferred, TRUE)
                ? ERROR_SUCCESS
                : GetLastError();

    return WaitResult == WAIT_OBJECT_0 + 1 ? ERROR_DEVICE_NOT_CONNECTED : Error;
}

/**
 * @brief Read one byte from the USB debuggee
 * @details Takes the byte from the queue that the reader thread fills in USB order.
 *
 * @param Handle
 * @param OutByte
 * @param BytesRead
 * @return BOOLEAN TRUE with no bytes read once after the USB device is lost
 */
BOOLEAN
DbcReadByte(HANDLE Handle, CHAR * OutByte, DWORD * BytesRead)
{
    PDBC_CONNECTION Connection  = NULL;
    BOOLEAN         ReaderEnded = FALSE;
    BOOLEAN         Result      = FALSE;

    *BytesRead = 0;

    Connection = DbcAcquireConnection(Handle);

    if (Connection == NULL)
    {
        return FALSE;
    }

    HANDLE Events[] = {Connection->CloseEvent, Connection->DataEvent, Connection->ReaderThread};

    EnterCriticalSection(&Connection->ReadLock);

    while (Connection->Closed == 0)
    {
        EnterCriticalSection(&Connection->QueueLock);

        if (Connection->QueueLength != 0)
        {
            *OutByte   = (CHAR)Connection->Queue[Connection->QueueHead];
            *BytesRead = 1;
            Result     = TRUE;

            Connection->QueueHead = (Connection->QueueHead + 1) % DBC_USB_READ_QUEUE_SIZE;
            Connection->QueueLength--;
        }
        else if (Connection->DeviceLost)
        {
            //
            // Return no bytes so that the caller discards the incomplete USB packet
            //
            Connection->DeviceLost = FALSE;
            Result                 = TRUE;
        }
        else if (!ReaderEnded)
        {
            Connection->ConsumerWaiting = TRUE;
            ResetEvent(Connection->DataEvent);
        }

        if (Result)
        {
            DbcWakeQueueReader(Connection);
        }

        LeaveCriticalSection(&Connection->QueueLock);

        if (Result || ReaderEnded)
        {
            break;
        }

        //
        // Stop after the queued bytes if the reader thread ends without a close
        //
        if (WaitForMultipleObjects(ARRAYSIZE(Events), Events, FALSE, INFINITE) == WAIT_OBJECT_0 + DBC_CONSUMER_EVENT_READER)
        {
            ReaderEnded = TRUE;
        }
    }

    LeaveCriticalSection(&Connection->ReadLock);
    DbcReleaseConnection(Connection);

    return Result;
}

/**
 * @brief Write to the debuggee in USB packet chunks
 *
 * @param Handle
 * @param Buffer
 * @param Length
 * @return BOOLEAN
 */
BOOLEAN
DbcWrite(HANDLE Handle, const VOID * Buffer, UINT32 Length)
{
    PDBC_CONNECTION Connection     = NULL;
    PDBC_USB_DEVICE Device         = NULL;
    UINT32          Offset         = 0;
    ULONG           ChunkSize      = 0;
    ULONG           Transferred    = 0;
    DWORD           LastError      = ERROR_SUCCESS;
    DWORD           CloseStart     = 0;
    DWORD           Elapsed        = 0;
    DWORD           WaitTimeout    = INFINITE;
    BOOLEAN         Closing        = FALSE;
    BOOLEAN         PipeReset      = FALSE;
    BOOLEAN         ResetSucceeded = FALSE;
    BOOLEAN         WriteFailed    = FALSE;
    BOOLEAN         Result         = FALSE;

    if (Length == 0)
    {
        return TRUE;
    }

    Connection = DbcAcquireConnection(Handle);

    if (Connection == NULL)
    {
        return FALSE;
    }

    Closing = g_SerialConnectionAlreadyClosed;

    //
    // Skip the final write if another writer holds the lock
    //
    if (Closing)
    {
        if (!TryEnterCriticalSection(&Connection->WriteLock))
        {
            DbcReleaseConnection(Connection);
            return FALSE;
        }

        CloseStart = GetTickCount();
    }
    else
    {
        EnterCriticalSection(&Connection->WriteLock);
    }

    while (Offset < Length && Connection->Closed == 0 &&
           (Closing || !g_SerialConnectionAlreadyClosed))
    {
        Device = DbcAcquireUsbDevice(Connection, TRUE);

        if (Device == NULL)
        {
            break;
        }

        ChunkSize = Length - Offset;

        if (ChunkSize > Device->PacketSizeOut)
        {
            ChunkSize = Device->PacketSizeOut;
        }

        PipeReset = FALSE;

        while (ChunkSize != 0 && Connection->Closed == 0 && Device->Failed == 0 &&
               (Closing || !g_SerialConnectionAlreadyClosed))
        {
            if (Closing)
            {
                Elapsed = GetTickCount() - CloseStart;

                if (Elapsed >= DBC_USB_WRITE_TIMEOUT_MS)
                {
                    break;
                }

                WaitTimeout = DBC_USB_WRITE_TIMEOUT_MS - Elapsed;
            }

            Transferred = 0;
            LastError   = DbcTransferUsbPipe(Connection,
                                            Device,
                                            Device->PipeOut,
                                            (PUCHAR)Buffer + Offset,
                                            ChunkSize,
                                            Connection->WriteEvent,
                                            WaitTimeout,
                                            &Transferred);

            if (Transferred > ChunkSize)
            {
                Transferred = ChunkSize;
            }

            if (LastError == ERROR_SUCCESS || LastError == ERROR_SEM_TIMEOUT)
            {
                Offset += Transferred;
                ChunkSize -= Transferred;

                if (Connection->Closed != 0 ||
                    (!Closing && g_SerialConnectionAlreadyClosed) ||
                    (Closing && (LastError != ERROR_SUCCESS || Transferred == 0)))
                {
                    break;
                }

                continue;
            }

            if (Connection->Closed != 0 || Closing || g_SerialConnectionAlreadyClosed)
            {
                WriteFailed = TRUE;
                break;
            }

            ResetSucceeded = FALSE;

            if (!PipeReset && DbcCanResetPipe(LastError))
            {
                ResetSucceeded = DbcTryResetPipe(Connection, Device, Device->PipeOut);
            }

            //
            // Retry only a zero-byte stall at the start of the write
            //
            if (ResetSucceeded && Offset == 0 && Transferred == 0 && LastError == ERROR_GEN_FAILURE)
            {
                PipeReset = TRUE;
            }
            else
            {
                if (!ResetSucceeded)
                {
                    DbcFailUsbDevice(Connection, Device);
                }

                WriteFailed = TRUE;
                break;
            }
        }

        DbcReleaseUsbDevice(Device);
        Device = NULL;

        if (WriteFailed || ChunkSize != 0 || (Closing && LastError != ERROR_SUCCESS))
        {
            break;
        }
    }

    Result = (Offset == Length) ? TRUE : FALSE;

    LeaveCriticalSection(&Connection->WriteLock);
    DbcReleaseConnection(Connection);

    return Result;
}

/**
 * @brief Cancel active transfers and close the USB debugger connection
 *
 * @param Handle
 * @return BOOLEAN
 */
BOOLEAN
DbcClose(HANDLE Handle)
{
    PDBC_CONNECTION Connection = NULL;
    PDBC_USB_DEVICE Device     = NULL;

    SpinlockLock(&g_DbcActiveLock);

    if (g_DbcActiveConnection != NULL &&
        (Handle == NULL || (HANDLE)g_DbcActiveConnection == Handle))
    {
        Connection            = g_DbcActiveConnection;
        g_DbcActiveConnection = NULL;
    }

    SpinlockUnlock(&g_DbcActiveLock);

    if (Connection == NULL)
    {
        return TRUE;
    }

    //
    // Wake transfer and reopen waits without taking the write lock
    //
    InterlockedExchange(&Connection->Closed, 1);
    SetEvent(Connection->CloseEvent);

    EnterCriticalSection(&Connection->DeviceLock);
    Device             = Connection->Device;
    Connection->Device = NULL;

    if (Device != NULL)
    {
        DbcAbortUsbPipes(Device);
    }

    LeaveCriticalSection(&Connection->DeviceLock);

    DbcReleaseUsbDevice(Device);

    //
    // Give the USB reader thread time to cancel its reads
    //
    if (WaitForSingleObject(Connection->ReaderThread, DBC_USB_READER_EXIT_TIMEOUT_MS) == WAIT_OBJECT_0)
    {
        DbcShowReadStatistics(Connection);
    }

    DbcReleaseConnection(Connection);

    return TRUE;
}

#else

/**
 * @brief Create a USB debugger connection
 *
 * @return HANDLE
 */
HANDLE
DbcConnect()
{
    ShowMessages("err, USB debugging is not supported on Linux\n");
    return NULL;
}

/**
 * @brief Read one byte from the USB debuggee
 *
 * @param Handle
 * @param OutByte
 * @param BytesRead
 * @return BOOLEAN
 */
BOOLEAN
DbcReadByte(HANDLE Handle, CHAR * OutByte, DWORD * BytesRead)
{
    UNREFERENCED_PARAMETER(Handle);
    UNREFERENCED_PARAMETER(OutByte);

    *BytesRead = 0;

    return FALSE;
}

/**
 * @brief Write a buffer to the USB debuggee
 *
 * @param Handle
 * @param Buffer
 * @param Length
 * @return BOOLEAN
 */
BOOLEAN
DbcWrite(HANDLE Handle, const VOID * Buffer, UINT32 Length)
{
    UNREFERENCED_PARAMETER(Handle);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(Length);

    return FALSE;
}

/**
 * @brief Close the USB debugger connection
 *
 * @param Handle
 * @return BOOLEAN
 */
BOOLEAN
DbcClose(HANDLE Handle)
{
    UNREFERENCED_PARAMETER(Handle);

    return TRUE;
}

#endif // _WIN32
