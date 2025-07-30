#pragma once

#include "cim_private.h"

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <windowsx.h>

#ifdef __cplusplus
extern "C" {
#endif

// NOTE: I don't think this should be exposed at all. But unsure if we provide 
// initialization functions or not...

void 
CimWin32_Log(CimLog_Level Level,
             const char  *File,
             cim_i32      Line,
             const char  *Format,
             ...);

LRESULT CALLBACK 
CimWin32_WindowProc(HWND Handle, UINT Message, WPARAM WParam, LPARAM LParam);

#ifdef __cplusplus
}
#endif

#endif // _WIN32
