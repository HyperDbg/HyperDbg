#include "WdmLib.h"

PCUNICODE_STRING getAnswer()
{
    static const UNICODE_STRING str = RTL_CONSTANT_STRING(L"42");
    return &str;
}