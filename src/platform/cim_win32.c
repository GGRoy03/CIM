#include "cim_win32.h"
#include "cim_private.h"

#include <stdio.h>
#include <stdarg.h>

#ifdef _WIN32


static inline void 
CimWin32_ProcessInputMessage(cim_io_button_state *NewState, bool IsDown)
{
    if(NewState->EndedDown != IsDown)
    {
        NewState->EndedDown = IsDown;
        ++NewState->HalfTransitionCount;
    }
}

#ifdef __cplusplus
extern "C" {
#endif

void 
CimWin32_Log(CimLog_Level Level,
             const char  *File,
             cim_i32      Line,
             const char  *Format,
             va_list      Args)
{
    char Buffer[1024] = { 0 };
    vsnprintf(Buffer, sizeof(Buffer), Format, Args);

    char FinalMessage[2048] = { 0 };
    snprintf(FinalMessage, sizeof(FinalMessage), "[%s:%d] %s\n", File, Line, Buffer);

    OutputDebugStringA(FinalMessage);
}

CimLogHandlerFunction *CimPlatform_Logger = CimWin32_Log;

LRESULT CALLBACK
CimWin32_WindowProc(HWND Handle, UINT Message, WPARAM WParam, LPARAM LParam)
{
    cim_context   *Ctx    = CimContext; if (!Ctx) return FALSE;
    cim_io_inputs *Inputs = &Ctx->Inputs;

    switch(Message)
    {

    case WM_MOUSEMOVE:
    {
        cim_i32 MouseX = GET_X_LPARAM(LParam);
        cim_i32 MouseY = GET_Y_LPARAM(LParam);

        Inputs->MouseDeltaX = (cim_f32)(MouseX - Inputs->MouseX);
        Inputs->MouseDeltaY = (cim_f32)(MouseY - Inputs->MouseY);

        Inputs->MouseX = MouseX;
        Inputs->MouseY = MouseY;
    } break;

    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    {
        cim_u32 VKCode  = (cim_u32)WParam;
        bool    WasDown = ((LParam & ((size_t)1 << 30)) != 0);
        bool    IsDown  = ((LParam & ((size_t)1 << 31)) == 0);

        if (WasDown != IsDown && VKCode < CIM_KEYBOARD_KEY_COUNT)
        {
            CimWin32_ProcessInputMessage(&Inputs->Buttons[VKCode], IsDown);
        }
     } break;

    case WM_LBUTTONDOWN:
    {
        CimWin32_ProcessInputMessage(&Inputs->MouseButtons[CimMouse_Left], true);
    } break;

    case WM_LBUTTONUP:
    {
        CimWin32_ProcessInputMessage(&Inputs->MouseButtons[CimMouse_Left], false);
    } break;

    case WM_RBUTTONDOWN:
    {
        CimWin32_ProcessInputMessage(&Inputs->MouseButtons[CimMouse_Right], true);
    } break;

    case WM_RBUTTONUP:
    {
        CimWin32_ProcessInputMessage(&Inputs->MouseButtons[CimMouse_Right], false);
    } break;

    case WM_MBUTTONDOWN:
    {
        CimWin32_ProcessInputMessage(&Inputs->MouseButtons[CimMouse_Middle], true);
    } break;

    case WM_MBUTTONUP:
    {
        CimWin32_ProcessInputMessage(&Inputs->MouseButtons[CimMouse_Middle], false);
    } break;

    case WM_MOUSEWHEEL:
    {
    } break;

    }

    return FALSE; // We don't want to block any messages right now.
}

#ifdef __cplusplus
}
#endif

#endif // _WIN32
