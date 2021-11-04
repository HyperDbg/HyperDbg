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
