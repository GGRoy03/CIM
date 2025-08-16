// [Public API]
static bool InitializePlatform(const char *StyleDir);
static void LogMessage(CimLog_Severity Level, const char *File, cim_i32 Line, const char *Format, ...);

// [Platform Agnostic Helpers]

static void
ProcessInputMessage(cim_button_state *NewState, bool IsDown)
{
    if (NewState->EndedDown != IsDown)
    {
        NewState->EndedDown = IsDown;
        ++NewState->HalfTransitionCount;
    }
}

bool
IsMouseDown(CimMouse_Button MouseButton, cim_inputs *Inputs)
{
    bool IsDown = Inputs->MouseButtons[MouseButton].EndedDown;

    return IsDown;
}

bool
IsMouseReleased(CimMouse_Button MouseButton, cim_inputs *Inputs)
{
    cim_button_state *State = &Inputs->MouseButtons[MouseButton];
    bool IsReleased = (!State->EndedDown) && (State->HalfTransitionCount > 0);

    return IsReleased;
}

bool
IsMouseClicked(CimMouse_Button MouseButton, cim_inputs *Inputs)
{
    cim_button_state *State = &Inputs->MouseButtons[MouseButton];
    bool IsClicked = (State->EndedDown) && (State->HalfTransitionCount > 0);

    return IsClicked;
}

cim_i32
GetMouseDeltaX(cim_inputs *Inputs)
{
    cim_i32 DeltaX = Inputs->MouseDeltaX;

    return DeltaX;
}

cim_i32
GetMouseDeltaY(cim_inputs *Inputs)
{
    cim_i32 DeltaY = Inputs->MouseDeltaY;

    return DeltaY;
}

cim_vector2
GetMousePosition(cim_inputs *Inputs)
{
    cim_vector2 Position = (cim_vector2){ (cim_f32)Inputs->MouseX, (cim_f32)Inputs->MouseY };

    return Position;
}

// [Win32]

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <windowsx.h>

// [Internals]
static DWORD   WINAPI   CimIOThreadProc  (LPVOID Param);
static LRESULT CALLBACK CimWindowProc    (HWND Handle, UINT Message, WPARAM WParam, LPARAM LParam);

bool
InitializePlatform(const char *StyleDir)
{
    // Initialize the IO Thread (The thread handles the resource cleanups)

    cim_file_watcher_context *IOContext = (cim_file_watcher_context *)calloc(1, sizeof(cim_file_watcher_context));
    if (!IOContext)
    {
        CimLog_Error("Win32 Init: Failed to allocate memory for the IO context");
        return false;
    }
    strncpy_s(IOContext->Directory, sizeof(IOContext->Directory),
        StyleDir, strlen(StyleDir));
    IOContext->Directory[(sizeof(IOContext->Directory) - 1)] = '\0';

    WIN32_FIND_DATAA FindData;
    HANDLE           FindHandle = INVALID_HANDLE_VALUE;

    char   SearchPattern[MAX_PATH];
    size_t DirLength = strlen(StyleDir);

    // Path/To/Dir[/*.cim]" 6 Char
    if (DirLength + 6 >= MAX_PATH)
    {
        CimLog_Error("Win32 Init: Provided directory is too long: %s", StyleDir);
        return false;
    }
    snprintf(SearchPattern, MAX_PATH, "%s/*.cim", StyleDir);

    FindHandle = FindFirstFileA(SearchPattern, &FindData);
    if (FindHandle == INVALID_HANDLE_VALUE)
    {
        DWORD Error = GetLastError();
        if (Error == ERROR_FILE_NOT_FOUND)
        {
            CimLog_Warn("Win32 Init: Provided directory: %s does not contain any .cim files", StyleDir);
            return true;
        }
        else
        {
            CimLog_Error("Win32 Init: FindFirstFileA Failed with error %u", Error);
            return false;
        }
    }

    const cim_u32 Capacity = 4;

    IOContext->Files = (cim_watched_file *)calloc(Capacity, sizeof(cim_watched_file));
    IOContext->FileCount = 0;
    if (!IOContext->Files)
    {
        CimLog_Error("Win32 Init: Failed to allocate memory for the wathched files.");
        return false;
    }

    do
    {
        if (!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            if (IOContext->FileCount == Capacity)
            {
                CimLog_Warn("Win32 Init: Maximum number of style files reached.");
                break;
            }

            cim_watched_file *Watched = IOContext->Files + IOContext->FileCount;

            size_t NameLength = strlen(FindData.cFileName);
            if (NameLength >= MAX_PATH)
            {
                CimLog_Error("Win32 Init: File name is too long: %s",
                    FindData.cFileName);
                return false;
            }

            memcpy(Watched->FileName, FindData.cFileName, NameLength);
            Watched->FileName[NameLength] = '\0';

            size_t FullLength = DirLength + 1 + NameLength + 1;
            if (FullLength >= MAX_PATH)
            {
                CimLog_Error("Win32 Init: Full path for %s is too long.",
                    FindData.cFileName);
                return false;
            }

            snprintf(Watched->FullPath, FullLength, "%s/%s", StyleDir,
                FindData.cFileName);
            Watched->FullPath[FullLength] = '\0';

            ++IOContext->FileCount;
        }
    } while (FindNextFileA(FindHandle, &FindData));
    FindClose(FindHandle);

    if (IOContext->FileCount > 0)
    {
        HANDLE IOThreadHandle = CreateThread(NULL, 0, CimIOThreadProc, IOContext, 0, NULL);
        if (!IOThreadHandle)
        {
            CimLog_Error("Win32 Init: Failed to launch IO Thread with error : %u", GetLastError());
            return false;
        }

        CloseHandle(IOThreadHandle);
    }

    // Call the style parser. Should provide the list of all of the sub-files
    // in the style directory. There could be some weird race error if the IO thread
    // exits early and frees the context. Right not just pass the first
    // one and don't worry about the race...

    InitializeStyle(IOContext->Files[0].FullPath);

    return true;
}

static void
LogMessage(CimLog_Severity Level, const char *File, cim_i32 Line, const char *Format, ...)
{
    Cim_Unused(Level);

    va_list Args;
    __crt_va_start(Args, Format);
    __va_start(&Args, Format);

    char Buffer[1024] = { 0 };
    vsnprintf(Buffer, sizeof(Buffer), Format, Args);

    char FinalMessage[2048] = { 0 };
    snprintf(FinalMessage, sizeof(FinalMessage), "[%s:%d] %s\n", File, Line, Buffer);

    OutputDebugStringA(FinalMessage);

    __crt_va_end(Args);
}

LRESULT CALLBACK
CimWindowProc(HWND Handle, UINT Message, WPARAM WParam, LPARAM LParam)
{
    Cim_Unused(Handle);

    if (!CimCurrent)
    {
        return FALSE;
    }
    cim_inputs *Inputs = UIP_INPUT;

    switch (Message)
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

        if (WasDown != IsDown && VKCode < CimIO_KeyboardKeyCount)
        {
            ProcessInputMessage(&Inputs->Buttons[VKCode], IsDown);
        }
    } break;

    case WM_LBUTTONDOWN:
    {
        ProcessInputMessage(&Inputs->MouseButtons[CimMouse_Left], true);
    } break;

    case WM_LBUTTONUP:
    {
        ProcessInputMessage(&Inputs->MouseButtons[CimMouse_Left], false);
    } break;

    case WM_RBUTTONDOWN:
    {
        ProcessInputMessage(&Inputs->MouseButtons[CimMouse_Right], true);
    } break;

    case WM_RBUTTONUP:
    {
        ProcessInputMessage(&Inputs->MouseButtons[CimMouse_Right], false);
    } break;

    case WM_MBUTTONDOWN:
    {
        ProcessInputMessage(&Inputs->MouseButtons[CimMouse_Middle], true);
    } break;

    case WM_MBUTTONUP:
    {
        ProcessInputMessage(&Inputs->MouseButtons[CimMouse_Middle], false);
    } break;

    case WM_MOUSEWHEEL:
    {
    } break;

    }

    return FALSE; // We don't want to block any messages right now.
}

static DWORD WINAPI
CimIOThreadProc(LPVOID Param)
{
    cim_file_watcher_context *WatchContext = (cim_file_watcher_context *)Param;

    HANDLE DirHandle =
        CreateFileA(WatchContext->Directory, FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

    if (DirHandle == INVALID_HANDLE_VALUE)
    {
        CimLog_Fatal("Win32 IO Thread: Failed to open directory : %s",
            WatchContext->Directory);
        return 1;
    }

    BYTE  Buffer[4096];
    DWORD BytesReturned = 0;

    while (true)
    {
        DWORD Filter = FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME |
            FILE_NOTIFY_CHANGE_CREATION;

        BOOL Okay = ReadDirectoryChangesW(DirHandle, Buffer, sizeof(Buffer),
            FALSE, Filter, &BytesReturned,
            NULL, NULL);
        if (!Okay)
        {
            CimLog_Error("ReadDirectoryChangesW failed: %u\n", GetLastError());
            break;
        }

        BYTE *Ptr = Buffer;
        do
        {
            FILE_NOTIFY_INFORMATION *Info = (FILE_NOTIFY_INFORMATION *)Ptr;

            char    Name[CimIO_MaxPath];
            cim_i32 Length = WideCharToMultiByte(CP_UTF8, 0,
                Info->FileName,
                Info->FileNameLength / sizeof(WCHAR),
                Name, sizeof(Name) - 1, NULL, NULL);
            Name[Length] = '\0';

            for (cim_u32 FileIdx = 0; FileIdx < WatchContext->FileCount; FileIdx++)
            {
                if (_stricmp(Name, WatchContext->Files[FileIdx].FileName) == 0)
                {
                    CimLog_Info("File has changed : %s",
                        WatchContext->Files[FileIdx].FullPath);
                    // CimStyle_Initialize(WatchContext->Files[FileIdx].FullPath); // ?
                    break;
                }
            }

            if (Info->NextEntryOffset == 0)
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

#endif