// [Headers]

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <windowsx.h>

#include <stdio.h>  // Formatting
#include <stdarg.h> // VA Args

#include "public/cim_public.h"
#include "private/cim_private.h"

// [Internal Functions]

static inline void
CimWin32_ProcessInputMessage(cim_button_state *NewState, bool IsDown)
{
    if (NewState->EndedDown != IsDown)
    {
        NewState->EndedDown = IsDown;
        ++NewState->HalfTransitionCount;
    }
}

// TODO:
// Debouncing
// Async?
// Threaded data-flow: Agnostic->Platform
// Checking into threading more.

static DWORD WINAPI
IOThreadProc(LPVOID Param)
{
    file_watcher_context *WatchContext = (file_watcher_context*)Param;

    HANDLE DirHandle = CreateFileA(WatchContext->Directory, FILE_LIST_DIRECTORY,
                                   FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                   NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if(DirHandle == INVALID_HANDLE_VALUE)
    {
        CimLog_Fatal("Win32 IO Thread: Failed to open directory provided by the user: %s", WatchContext->Directory);
        return 1;
    }

    BYTE  Buffer[4096];
    DWORD BytesReturned = 0;

    while(true)
    {
        DWORD Filter = FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_CREATION;
        BOOL  Okay   = ReadDirectoryChangesW(DirHandle, Buffer, sizeof(Buffer),
                                             FALSE, Filter , &BytesReturned,
                                             NULL, NULL);
        if(!Okay)
        {
            CimLog_Error("ReadDirectoryChangesW failed: %u\n", GetLastError());
            break;
        }

        BYTE *Ptr = Buffer;
        do
        {
            FILE_NOTIFY_INFORMATION *Info = (FILE_NOTIFY_INFORMATION*)Ptr;

            char    Name[CimIO_MaxPath];
            cim_i32 Length = WideCharToMultiByte(CP_UTF8, 0,
                                                 Info->FileName, Info->FileNameLength / sizeof(WCHAR),
                                                 Name, sizeof(Name) -1, NULL, NULL);
            Name[Length] = '\0';

            for(cim_u32 FileIdx = 0; FileIdx < WatchContext->FileCount; FileIdx++)
            {
                if(_stricmp(Name, WatchContext->Files[FileIdx].FileName) == 0)
                {
                    // NOTE: Obviously this is really naive and just completely reparses the file 
                    // which is okay for now. (Probably do not provide this function to the user?)
                    // It's also weirdly named. I need to clarify the data flow and rewrite how the
                    // thread works, but the boilerplate is mostly there.

                    CimLog_Info("File has changed : %s", WatchContext->Files[FileIdx].FullPath);
                    CimStyle_Initialize(WatchContext->Files[FileIdx].FullPath);
                    break;
                }
            }

            if(Info->NextEntryOffset == 0)
            {
                break;
            }

            Ptr += Info->NextEntryOffset;
        } while (true);

    }

    CloseHandle(DirHandle);
    free(WatchContext->Files);
    free(WatchContext);

    return 0;
}

// [API Implementation]

#ifdef __cplusplus
extern "C" {
#endif

#define _CRT_SECURE_NO_WARNINGS_

bool
CimWin32_Initialize(const char *StyleDirectory)
{
    // Initialize the IO Thread (The thread handles the resource cleanups)

    file_watcher_context *IOContext = calloc(1, sizeof(file_watcher_context));
    if(!IOContext)
    {
        CimLog_Error("Win32 Init: Failed to allocate memory for the IO context");
        return false;
    }
    strncpy_s(IOContext->Directory, sizeof(IOContext->Directory), StyleDirectory, strlen(StyleDirectory)); // NOTE: Get rid of MSVC?
    IOContext->Directory[(sizeof(IOContext->Directory) - 1)] = '\0';

    WIN32_FIND_DATAA FindData;
    HANDLE           FindHandle = INVALID_HANDLE_VALUE;

    char   SearchPattern[MAX_PATH];
    size_t DirLength = strlen(StyleDirectory);

    // Path/To/Dir[/*.cim]" 6 Char
    if(DirLength + 6  >= MAX_PATH)
    {
        CimLog_Error("Win32 Init: Provided directory is too long: %s", StyleDirectory);
        return false;
    }
    snprintf(SearchPattern, MAX_PATH, "%s/*.cim", StyleDirectory);

    FindHandle = FindFirstFileA(SearchPattern, &FindData);
    if(FindHandle == INVALID_HANDLE_VALUE)
    {
        DWORD Error = GetLastError();
        if(Error == ERROR_FILE_NOT_FOUND)
        {
            CimLog_Warn("Win32 Init: Provided directory: %s does not contain any .cim files", StyleDirectory);
            return true;
        }
        else
        {
            CimLog_Error("Win32 Init: FindFirstFileA Failed with error %u", Error);
            return false;
        }
    }

    const cim_u32 Capacity = 4;

    IOContext->Files     = calloc(Capacity, sizeof(watched_file));
    IOContext->FileCount = 0;
    if (!IOContext->Files)
    {
        CimLog_Error("Win32 Init: Failed to allocate memory for the wathched files.");
        return false;
    }

    do
    {
        if(!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            if(IOContext->FileCount == Capacity)
            {
                CimLog_Warn("Win32 Init: Maximum number of style files reached.");
                break;
            }

            watched_file *Watched = IOContext->Files + IOContext->FileCount;

            size_t NameLength = strlen(FindData.cFileName);
            if (NameLength >= MAX_PATH)
            {
                CimLog_Error("Win32 Init: File name is too long: %s", FindData.cFileName);
                return false;
            }

            memcpy(Watched->FileName, FindData.cFileName, NameLength);
            Watched->FileName[NameLength] = '\0';

            size_t FullLength = DirLength + 1 + NameLength + 1;
            if (FullLength >= MAX_PATH)
            {
                CimLog_Error("Win32 Init: Full path for %s is too long.", FindData.cFileName);
                return false;
            }

            snprintf(Watched->FullPath, FullLength, "%s/%s", StyleDirectory, FindData.cFileName);
            Watched->FullPath[FullLength] = '\0';

            ++IOContext->FileCount;
        }
    } while(FindNextFileA(FindHandle, &FindData));
    FindClose(FindHandle);

    if(IOContext->FileCount > 0)
    {
        HANDLE IOThreadHandle = CreateThread(NULL, 0, IOThreadProc, IOContext, 0, NULL);
        if (!IOThreadHandle)
        {
            CimLog_Error("Win32 Init: Failed to launch IO Thread with error : %u", GetLastError());
            return false;
        }

        CloseHandle(IOThreadHandle);
    }

    return true;
}

LRESULT CALLBACK 
CimWin32_WindowProc(HWND Handle, UINT Message, WPARAM WParam, LPARAM LParam)
{
    cim_context *Ctx = CimContext; 

    if(!Ctx)
    {
        return FALSE;
    }

    cim_inputs *Inputs = &Ctx->Inputs;

    switch(Message)
    {

    case WM_MOUSEMOVE:
    {
        cim_i32 MouseX = GET_X_LPARAM(LParam);
        cim_i32 MouseY = GET_Y_LPARAM(LParam);

        Inputs->MouseDeltaX += (MouseX - Inputs->MouseX);
        Inputs->MouseDeltaY += (MouseY - Inputs->MouseY);

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

void 
CimWin32_LogMessage(CimLog_Severity Level, const char *File, cim_i32 Line,
                    const char *Format, va_list Args)
{
    char Buffer[1024] = { 0 };
    vsnprintf(Buffer, sizeof(Buffer), Format, Args);

    char FinalMessage[2048] = { 0 };
    snprintf(FinalMessage, sizeof(FinalMessage), "[%s:%d] %s\n", File, Line, Buffer);

    OutputDebugStringA(FinalMessage);
}

CimLogFn *CimLogger = CimWin32_LogMessage;

#ifdef __cplusplus
}
#endif

#endif
