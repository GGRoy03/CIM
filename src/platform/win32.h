#pragma once

#include "private/cim_private.h"

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <windowsx.h>

#ifdef __cplusplus
extern "C" {
#endif

LRESULT CALLBACK 
CimWin32_WindowProc(HWND Handle, UINT Message, WPARAM WParam, LPARAM LParam);

bool CimWin32_Initialize(const char *StyleDirectory);

#ifdef __cplusplus
}
#endif

#endif // _WIN32
