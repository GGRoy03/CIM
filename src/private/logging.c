#include "cim_private.h"

#include <stdarg.h> // va_list

#ifdef __cplusplus
extern "C" {
#endif

void
Cim_Log(CimLog_Severity Level, const char *File, cim_i32 Line, const char *Format, ...)
{
    if (!*CimLogger)
    {
        return;
    }
    else
    {
        va_list Args;
        __crt_va_start(Args, Format);
        __va_start(&Args, Format);
        CimLogger(Level, File, Line, Format, Args);
        __crt_va_end(Args);
    }
}

#ifdef __cplusplus
}
#endif