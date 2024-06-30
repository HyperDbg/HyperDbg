#include "KmdfCppLib.h"

#include <stdio.h>

WDFSTRING getAnswer()
{
    static wchar_t answer[3];
    _snwprintf_s(answer, 3, L"%s", L"42");

    static const UNICODE_STRING str = RTL_CONSTANT_STRING(answer);

    WDFSTRING wdfstr = nullptr;
    NTSTATUS status = WdfStringCreate(&str, WDF_NO_OBJECT_ATTRIBUTES, &wdfstr);
    if (!NT_SUCCESS(status))
    {
        return nullptr;
    }

    return wdfstr;
}