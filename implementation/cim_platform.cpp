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

static bool
IsMouseDown(CimMouse_Button MouseButton, cim_inputs *Inputs)
{
    bool IsDown = Inputs->MouseButtons[MouseButton].EndedDown;

    return IsDown;
}

static bool
IsMouseClicked(CimMouse_Button MouseButton, cim_inputs *Inputs)
{
    cim_button_state *State = &Inputs->MouseButtons[MouseButton];
    bool IsClicked = (State->EndedDown) && (State->HalfTransitionCount > 0);

    return IsClicked;
}

static cim_i32
GetMouseDeltaX(cim_inputs *Inputs)
{
    cim_i32 DeltaX = Inputs->MouseDeltaX;

    return DeltaX;
}

static cim_i32
GetMouseDeltaY(cim_inputs *Inputs)
{
    cim_i32 DeltaY = Inputs->MouseDeltaY;

    return DeltaY;
}

// [Win32]

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <windowsx.h>

// [Internals]

// NOTE: These should all be Win32Something right? Or? Platform? If public??
static DWORD   WINAPI   Win32WatcherThread  (LPVOID Param);
static LRESULT CALLBACK Win32CimProc        (HWND Handle, UINT Message, WPARAM WParam, LPARAM LParam);

// [Public API Implementation]

static bool
PlatformInit(const char *StyleDir)
{
    dir_watcher_context *Watcher = (dir_watcher_context *)calloc(1, sizeof(dir_watcher_context));
    if (!Watcher)
    {
        CimLog_Error("Win32 Init: Failed to allocate memory for the IO context");
        return false;
    }
    strncpy_s(Watcher->Directory, sizeof(Watcher->Directory),
              StyleDir, strlen(StyleDir));
    Watcher->Directory[(sizeof(Watcher->Directory) - 1)] = '\0';
    Watcher->RefCount = 2; // NOTE: Need it on the main thread as well as the watcher thread.

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

    Watcher->Files     = (cim_watched_file *)calloc(Capacity, sizeof(cim_watched_file));
    Watcher->FileCount = 0;

    if (!Watcher->Files)
    {
        CimLog_Error("Win32 Init: Failed to allocate memory for the wathched files.");
        return false;
    }

    do
    {
        if (!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            if (Watcher->FileCount == Capacity)
            {
                CimLog_Warn("Win32 Init: Maximum number of style files reached.");
                break;
            }

            cim_watched_file *Watched = Watcher->Files + Watcher->FileCount;

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

            ++Watcher->FileCount;
        }
    } while (FindNextFileA(FindHandle, &FindData));
    FindClose(FindHandle);

    if (Watcher->FileCount > 0)
    {
        HANDLE IOThreadHandle = CreateThread(NULL, 0, Win32WatcherThread, Watcher, 0, NULL);
        if (!IOThreadHandle)
        {
            CimLog_Error("Win32 Init: Failed to launch IO Thread with error : %u", GetLastError());
            return false;
        }

        CloseHandle(IOThreadHandle);

        // BUG: This is incorrect. Perhpas the LoadThemeFiles function should accept
        // a watched file as argument? Forced to pass 1 right now to circumvent it.
        char *P = Watcher->Files[0].FullPath;
        LoadThemeFiles(&P, 1);
    }

    if (InterlockedDecrement(&Watcher->RefCount) == 0)
    {
        if( Watcher->Files) free(Watcher->Files);
        if (Watcher)        free(Watcher);
    }

    return true;
}

static buffer
PlatformReadFile(char *FileName)
{
    // TODO: Make this a buffered read.

    buffer Result = {};

    if (!FileName)
    {
        CimLog_Error("ReadEntireFile: FileName is NULL");
        return Result;
    }

    HANDLE hFile = CreateFileA(FileName,
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        CimLog_Error("Unable to open file: %s", FileName);
        return Result;
    }

    LARGE_INTEGER FileSizeLI = {};
    if (!GetFileSizeEx(hFile, &FileSizeLI))
    {
        CimLog_Error("GetFileSizeEx failed for: %s", FileName);
        CloseHandle(hFile);
        return Result;
    }

    if (FileSizeLI.QuadPart == 0)
    {
        CloseHandle(hFile);
        return Result;
    }

    size_t FileSize = (size_t)FileSizeLI.QuadPart;

    Result = AllocateBuffer(FileSize + 1);
    if (!IsValidBuffer(&Result) || !Result.Data)
    {
        CimLog_Error("Unable to allocate buffer for file: %s", FileName);
        CloseHandle(hFile);
        return Result;
    }

    DWORD BytesToRead = (DWORD)FileSize;
    DWORD BytesRead   = 0;
    BOOL ok = ReadFile(hFile, Result.Data, BytesToRead, &BytesRead, NULL);
    if (!ok || BytesRead == 0)
    {
        CimLog_Error("ReadFile failed or read zero bytes for: %s", FileName);
        FreeBuffer(&Result);
        Result.Data = NULL;
        Result.Size = 0;
        Result.At = 0;
        CloseHandle(hFile);
        return Result;
    }

    ((char *)Result.Data)[BytesRead] = '\0';

    Result.Size = BytesRead;
    Result.At = 0;

    CloseHandle(hFile);
    return Result;
}

static void
PlatformLogMessage(CimLog_Severity Level, const char *File, cim_i32 Line, const char *Format, ...)
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

// [Internals Implementation]

LRESULT CALLBACK
Win32CimProc(HWND Handle, UINT Message, WPARAM WParam, LPARAM LParam)
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
        cim_u32 VKCode = (cim_u32)WParam;
        bool    WasDown = ((LParam & ((size_t)1 << 30)) != 0);
        bool    IsDown = ((LParam & ((size_t)1 << 31)) == 0);

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
Win32WatcherThread(LPVOID Param)
{
    dir_watcher_context *Watcher = (dir_watcher_context *)Param;

    HANDLE DirHandle = CreateFileA(Watcher->Directory, FILE_LIST_DIRECTORY,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                   NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

    if (DirHandle == INVALID_HANDLE_VALUE)
    {
        CimLog_Fatal("Failed to open directory : %s", Watcher->Directory);
        return 1;
    }

    DWORD BytesReturned = 0;
    BYTE  Buffer[4096]  = {};

    while (true)
    {
        DWORD Filter = FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_CREATION;

        BOOL FoundUpdate = ReadDirectoryChangesW(DirHandle, Buffer, sizeof(Buffer),
                                                 FALSE, Filter, &BytesReturned,
                                                 NULL, NULL);
        if (!FoundUpdate)
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

            for (cim_u32 FileIdx = 0; FileIdx < Watcher->FileCount; FileIdx++)
            {
                if (_stricmp(Name, Watcher->Files[FileIdx].FileName) == 0)
                {
                    // WARN:
                    // 1) We only handle modifications right now. Should also handle deletion
                    //    and new files.
                    // 2) It's better to batch them. Should accumulate then send the batch.

                    if (Info->Action == FILE_ACTION_MODIFIED)
                    {
                        char *FileToChange = Watcher->Files[FileIdx].FullPath;
                        LoadThemeFiles(&FileToChange, 1);
                    }

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

    if (InterlockedDecrement(&Watcher->RefCount) == 0)
    {
        if (Watcher->Files) free(Watcher->Files);
        if (Watcher)        free(Watcher);
    }

    return 0;
}

#endif
