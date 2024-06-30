#include <fltKernel.h>

PFLT_FILTER g_filterHandle = nullptr;

NTSTATUS FLTAPI instanceSetup(
    _In_ PCFLT_RELATED_OBJECTS          /*fltObjects*/,
    _In_ FLT_INSTANCE_SETUP_FLAGS       /*flags*/,
    _In_ DEVICE_TYPE                    /*volumeDeviceType*/,
    _In_ FLT_FILESYSTEM_TYPE            /*volumeFilesystemType*/
)
{
    return STATUS_SUCCESS;
}

NTSTATUS FLTAPI queryTeardown(
    _In_ PCFLT_RELATED_OBJECTS              /*fltObjects*/,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS  /*flags*/
)
{
    return STATUS_SUCCESS;
}

FLT_PREOP_CALLBACK_STATUS FLTAPI preCreate(
    _Inout_ PFLT_CALLBACK_DATA          /*data*/,
    _In_    PCFLT_RELATED_OBJECTS       /*fltObjects*/,
    _Outptr_result_maybenull_ PVOID*    /*completionContext*/
)
{
    DbgPrint("preCreate\n");

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

NTSTATUS FLTAPI filterUnloadCallback(_In_ FLT_FILTER_UNLOAD_FLAGS)
{
    DbgPrint("filterUnloadCallback\n");

    FltUnregisterFilter(g_filterHandle);
    return STATUS_SUCCESS;
}

extern "C" DRIVER_INITIALIZE DriverEntry;
extern "C" NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT driverObject, _In_ PUNICODE_STRING /*registryPath*/)
{
    DbgPrint("Driver loaded\n");

    const FLT_CONTEXT_REGISTRATION contextRegistration[] =
    {
        { FLT_CONTEXT_END }
    };

    const FLT_OPERATION_REGISTRATION callbacksRegistration[] =
    {
        {
            IRP_MJ_CREATE,
            0,
            &preCreate,
            nullptr
        },

        { IRP_MJ_OPERATION_END }
    };

    const FLT_REGISTRATION filterRegistration =
    {
        sizeof(FLT_REGISTRATION),           // Size
        FLT_REGISTRATION_VERSION,           // Version
        0,                                  // Flags
        contextRegistration,                // Context
        callbacksRegistration,              // Operation callbacks
        filterUnloadCallback,               // Mini filter unload 
        &instanceSetup,                     // InstanceSetup
        &queryTeardown,                     // InstanceQueryTeardown
        nullptr,                            // InstanceTeardownStart
        nullptr,                            // InstanceTeardownComplete
        nullptr,                            // GenerateFileNameCallback
        nullptr,                            // NormalizeNameComponentCallback
        nullptr,                            // NormalizeContextCleanupCallback
        nullptr,                            // TransactionNotificationCallback
        nullptr                             // NormalizeNameComponentExCallback
    };
    
    NTSTATUS status = FltRegisterFilter(driverObject, &filterRegistration, &g_filterHandle);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = FltStartFiltering(g_filterHandle);
    if (!NT_SUCCESS(status))
    {
        FltUnregisterFilter(g_filterHandle);
        return status;
    }

    return STATUS_SUCCESS;
}