#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <windowsx.h>

#ifdef __cplusplus
extern "C" {
#endif

LRESULT CALLBACK CimWin32_WindowProc(HWND Handle, UINT Message, WPARAM WParam, LPARAM LParam);

#ifdef __cplusplus
}
#endif
