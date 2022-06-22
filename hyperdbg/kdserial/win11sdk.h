#pragma once

__declspec(dllexport) void HviGetDebugDeviceOptions();
__declspec(dllexport) void HviGetHypervisorFeatures();
__declspec(dllexport) void HviIsHypervisorVendorMicrosoft();

inline void
HviGetDebugDeviceOptions()
{
    return;
}
inline void
HviGetHypervisorFeatures()
{
    return;
}
inline void
HviIsHypervisorVendorMicrosoft()
{
    return;
}

// -------------------------------------------------------------------- Globals

BOOLEAN
UsifInitializePort(
    _In_opt_ _Null_terminated_ PCHAR LoadOptions,
    _Inout_ PCPPORT                  Port,
    BOOLEAN                          MemoryMapped,
    UCHAR                            AccessSize,
    UCHAR                            BitWidth);

BOOLEAN
UsifSetBaud(
    _Inout_ PCPPORT Port,
    ULONG           Rate);

UART_STATUS
UsifGetByte(
    _Inout_ PCPPORT Port,
    _Out_ PUCHAR    Byte);

UART_STATUS
UsifPutByte(
    _Inout_ PCPPORT Port,
    UCHAR           Byte,
    BOOLEAN         BusyWait);

BOOLEAN
UsifRxReady(
    _Inout_ PCPPORT Port);
