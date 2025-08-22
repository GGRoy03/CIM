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

#include "d2d1.h"
#include "dwrite.h"

#pragma comment (lib, "dwrite")
#pragma comment (lib, "d2d1")

// [Internals]

// WARN: Temporary globals for simplicity.
static ID2D1RenderTarget *DWriteRenderTarget;
static ID2D1SolidColorBrush *DWriteFillBrush;
// We should cache by font?
static IDWriteFactory *DWriteFactory;
static IDWriteFontFace *FontFace;
static IDWriteTextFormat *TextFormat;

static DWORD   WINAPI   Win32WatcherThread(LPVOID Param);
static LRESULT CALLBACK Win32CimProc(HWND Handle, UINT Message, WPARAM WParam, LPARAM LParam);
static int              Win32UTF8ToWideString(char *UTF8String, WCHAR *WideString, cim_i32 WideStringBufferSize);
static void             Win32ReleaseFont();

typedef class win32_glyph_catcher : public IDWriteTextRenderer 
{

public:
    win32_glyph_catcher(text_layout_info *Out) : RefCount_(1), Out_(Out) {}
    virtual ~win32_glyph_catcher() = default;

    IFACEMETHODIMP QueryInterface(REFIID riid, void **Object) 
        override 
    {
        if (!Object) return E_POINTER;

        if (riid == __uuidof(IUnknown)            ||
            riid == __uuidof(IDWriteTextRenderer) ||
            riid == __uuidof(IDWritePixelSnapping) ) 
        {
            *Object = static_cast<IDWriteTextRenderer *>(this);
            this->AddRef();

            return S_OK;
        }

        *Object = nullptr;
        return E_NOINTERFACE;
    }

    IFACEMETHODIMP_(ULONG) AddRef()  
        override 
    { 
        return InterlockedIncrement(&RefCount_); 
    }

    IFACEMETHODIMP_(ULONG) Release() 
        override 
    {
        ULONG u = InterlockedDecrement(&RefCount_);
        if (u == 0) delete this;
        return u;
    }

    IFACEMETHODIMP IsPixelSnappingDisabled(void *ClientDrawingContext, BOOL *IsDisabled) 
        noexcept override
    {
        Cim_Unused(ClientDrawingContext);

        *IsDisabled = FALSE;
        return S_OK;
    }

    IFACEMETHODIMP GetCurrentTransform(void *ClientDrawingContext, DWRITE_MATRIX *Transform) 
        noexcept override
    {
        Cim_Unused(ClientDrawingContext);

        if (!Transform) return E_POINTER;

        Transform->m11 = 1.0f; Transform->m12 = 0.0f;
        Transform->m21 = 0.0f; Transform->m22 = 1.0f;
        Transform->dx  = 0.0f; Transform->dy = 0.0f;

        return S_OK;
    }

    IFACEMETHODIMP GetPixelsPerDip(void *ClientDrawingContext, FLOAT *PixelsPerDip) 
        noexcept override 
    {
        Cim_Unused(ClientDrawingContext);

        *PixelsPerDip = 1.0f;
        return S_OK;
    }

    IFACEMETHODIMP DrawGlyphRun(void *ClientDrawingContext, FLOAT /* BaselineOriginX */, FLOAT /* BaselineOriginY */,
                                DWRITE_MEASURING_MODE /* MeasuringMode */, const DWRITE_GLYPH_RUN *GlyphRun,
                                const DWRITE_GLYPH_RUN_DESCRIPTION *GlyphRunDescription, IUnknown *ClientDrawingEffect) 
        noexcept override
    {
        Cim_Unused(ClientDrawingContext);
        Cim_Unused(ClientDrawingEffect);
        Cim_Unused(GlyphRunDescription);

        // NOTE: bidi level is useful to know if we are drawing left to right or right to left.

        if (!GlyphRun || !GlyphRun->fontFace || !Out_) return E_FAIL;

        UINT    GlyphCount = GlyphRun->glyphCount;
        HRESULT Error      = S_OK;

        // WARN: Should not be done here, because it relates to the font.
        // So on font create. We need some font info struct probably.
        DWRITE_LINE_SPACING_METHOD Method;
        FLOAT                      LineSpacing;
        FLOAT                      Baseline;
        Error = TextFormat->GetLineSpacing(&Method, &LineSpacing, &Baseline);
        if (FAILED(Error))
        {
            return E_FAIL;
        }
        Out_->LineHeight = (cim_u32)LineSpacing;
        Cim_Assert(Out_->LineHeight == LineSpacing);


        for (UINT32 Idx = 0; Idx < GlyphCount; ++Idx)
        {
            glyph_layout_info *GlyphInfo = Out_->GlyphLayoutInfo + Idx;

            GlyphInfo->AdvanceX = GlyphRun->glyphAdvances[Idx];

            if (GlyphRun->glyphOffsets)
            {
                GlyphInfo->OffsetX = GlyphRun->glyphOffsets[Idx].advanceOffset;
                GlyphInfo->OffsetY = GlyphRun->glyphOffsets[Idx].ascenderOffset;
            }
            else
            {
                GlyphInfo->OffsetX = 0;
                GlyphInfo->OffsetY = 0;
            }
        }

        return S_OK;
    }

    IFACEMETHODIMP DrawUnderline      (void *, FLOAT, FLOAT, const DWRITE_UNDERLINE *, IUnknown *)          noexcept override { return S_OK; }
    IFACEMETHODIMP DrawStrikethrough  (void *, FLOAT, FLOAT, const DWRITE_STRIKETHROUGH *, IUnknown *)      noexcept override { return S_OK; }
    IFACEMETHODIMP DrawInlineObject   (void *, FLOAT, FLOAT, IDWriteInlineObject *, BOOL, BOOL, IUnknown *) noexcept override { return S_OK; }

private:
    long              RefCount_;
    text_layout_info *Out_;

} win32_glyph_catcher;

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

    // Simply Init DWrite here.
    HRESULT Error = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown **)&DWriteFactory);
    if (FAILED(Error) || !DWriteFactory)
    {
        CimLog_Error("Failed to create the factory for DWrite.");
        return false;
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

static void
PlatformSetTextObjects(IDXGISurface *TransferSurface)
{
    HRESULT Error = S_OK;

    ID2D1Factory        *Factory = NULL;
    D2D1_FACTORY_OPTIONS Options = { D2D1_DEBUG_LEVEL_ERROR };
    Error = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory), &Options, (void **)&Factory);
    if (FAILED(Error) || !Factory)
    {
        CimLog_Error("Could not create the factory for D2D1.");
        return;
    }

    D2D1_RENDER_TARGET_PROPERTIES Props =
        D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        0, 0);

    Error = Factory->CreateDxgiSurfaceRenderTarget(TransferSurface, &Props, &DWriteRenderTarget);
    if (FAILED(Error) || !DWriteRenderTarget)
    {
        CimLog_Error("Failed to create a render target for D2D1.");
        return;
    }

    Error = DWriteRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0F, 1.0F, 1.0F, 1.0F), &DWriteFillBrush);
    if (FAILED(Error) || !DWriteFillBrush)
    {
        CimLog_Error("Failed to create the fill brush for D2D1.");
        return;
    }

    Factory->Release();
}

static void
PlatformReleaseTextObjects()
{
    if (DWriteFillBrush)
    {
        DWriteFillBrush->Release();
        DWriteFillBrush = NULL;
    }

    if (DWriteRenderTarget)
    {
        DWriteRenderTarget->Release();
        DWriteRenderTarget = NULL;
    }
}

static void
PlatformSetFont(char *FontName, cim_u32 FontHeight)
{
    Win32ReleaseFont();

    if (DWriteFactory)
    {
        WCHAR WideFontName[256];
        if (FontName)
        {
            int Result = MultiByteToWideChar(CP_UTF8, 0, FontName,  -1,
                                             WideFontName, sizeof(WideFontName) / sizeof(WCHAR));

            if (Result == 0)
            {
                wcscpy_s(WideFontName, sizeof(WideFontName) / sizeof(WCHAR), L"Consolas");
            }
        }
        else
        {
            wcscpy_s(WideFontName, sizeof(WideFontName) / sizeof(WCHAR), L"Consolas");
        }

        // NOTE: What is the locale?
        DWriteFactory->CreateTextFormat(WideFontName, NULL, DWRITE_FONT_WEIGHT_REGULAR,
                                        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                        (cim_f32)FontHeight, L"en-us", &TextFormat);
        if (TextFormat)
        {
            // NOTE: That means for alignment we need one per glyph run again... That's so stupid?
            // I am for sure missing something. Maybe I can cheat it myself.. Like if alignment center 
            // give it a different layOut box? Idk.

            TextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
            TextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        }
    }
}

static text_layout_info
OSCreateTextLayout(char *String, cim_u32 Width, cim_u32 Height)
{
    text_layout_info LayoutInfo = {};
    HRESULT          Error      = S_OK;

    if (DWriteFactory)
    {
        // WARN: Probably don't want to set an hardcoded limit like that.
        WCHAR WideString[1024];
        int   WideSize = Win32UTF8ToWideString(String, WideString, 1024);
        if (WideSize == 0)
        {
            return LayoutInfo;
        }

        IDWriteTextLayout *TextLayout = NULL;
        Error = DWriteFactory->CreateTextLayout(WideString, WideSize, TextFormat,
                                                Width, Height, &TextLayout);
        if (FAILED(Error) || !TextLayout)
        {
            return LayoutInfo;
        }

        DWRITE_TEXT_METRICS Metrics = {};
        Error = TextLayout->GetMetrics(&Metrics);
        if (FAILED(Error))
        {
            return LayoutInfo;
        }

        size_t UTF8StringLength = strlen(String);
        LayoutInfo.GlyphCount      = (cim_u32)UTF8StringLength; Cim_Assert(UTF8StringLength == LayoutInfo.GlyphCount);
        LayoutInfo.GlyphLayoutInfo = (glyph_layout_info*)calloc(LayoutInfo.GlyphCount, sizeof(LayoutInfo.GlyphLayoutInfo[0]));
        win32_glyph_catcher Catcher(&LayoutInfo);
        
        // Need to call whatever.
        Error = TextLayout->Draw(NULL, &Catcher, 0.0f, 0.0f);
        if (FAILED(Error))
        {
            return LayoutInfo;
        }

        LayoutInfo.Width         = (cim_u32)(Metrics.width + 0.5f);
        LayoutInfo.Height        = (cim_u32)(Metrics.height + 0.5f);
        LayoutInfo.BackendLayout = TextLayout;

    }  

    return LayoutInfo;
}

static glyph_size
OSGetTextExtent(char *String, cim_u32 StringLength)
{
    // WARN: Obviously setting a hard limit on the string is trash, what can we do?

    glyph_size Result = {};

    if (DWriteFactory || StringLength >= 64)
    {
        WCHAR   WideString[64];
        cim_i32 WideSize = Win32UTF8ToWideString(String, WideString, 64);
        if (WideSize == 0)
        {
            return Result;
        }

        IDWriteTextLayout *TextLayout = NULL;
        HRESULT Error = DWriteFactory->CreateTextLayout(WideString, WideSize, TextFormat, 1024, 1024, &TextLayout);
        if (FAILED(Error) || !TextLayout)
        {
            return Result;
        }

        DWRITE_TEXT_METRICS Metrics = {};
        TextLayout->GetMetrics(&Metrics);
        Cim_Assert(Metrics.left == 0);
        Cim_Assert(Metrics.top  == 0);

        Result.Width  = (cim_u16)(Metrics.width  + 0.5f);
        Result.Height = (cim_u16)(Metrics.height + 0.5f);
    }

    return Result;
}

static void
OSRasterizeGlyph(char Character, stbrp_rect Rect)
{
    if (!DWriteRenderTarget || !DWriteFillBrush)
    {
        return;
    }

    WCHAR   WideCharacter[2];
    cim_i32 WideSize = Win32UTF8ToWideString(&Character, WideCharacter, 2);
    if (WideSize == 0)
    {
        return;
    }

    // NOTE: What should this be?
    D2D1_RECT_F DrawRect;
    DrawRect.left   = 0;
    DrawRect.top    = 0;
    DrawRect.right  = 0;
    DrawRect.bottom = 0;

    DWriteRenderTarget->SetTransform(D2D1::Matrix3x2F::Scale(D2D1::Size(1, 1),
                                     D2D1::Point2F(0.0f, 0.0f)));
    DWriteRenderTarget->BeginDraw();
    DWriteRenderTarget->Clear();
    DWriteRenderTarget->DrawTextW(WideCharacter, WideSize, TextFormat, &DrawRect, DWriteFillBrush, 
                                  D2D1_DRAW_TEXT_OPTIONS_CLIP | D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT,
                                  DWRITE_MEASURING_MODE_NATURAL);

    HRESULT Error = DWriteRenderTarget->EndDraw();
    if (FAILED(Error))
    {
        Cim_Assert(!"EndDraw failed.");
    }

    UI_RENDERER.TransferGlyph(Rect);
}

// [Internals Implementation]

static int
Win32UTF8ToWideString(char *UTF8String, WCHAR *WideString, cim_i32 WideStringBufferSize)
{
    if (!UTF8String || !WideString)
    {
        return 0;
    }

    int Needed = MultiByteToWideChar(CP_UTF8, 0, UTF8String, -1, NULL, 0);
    if (Needed <= 0 || Needed >= WideStringBufferSize)
    {
        return 0;
    }

    MultiByteToWideChar(CP_UTF8, 0, UTF8String, -1, WideString, Needed);

    return Needed;
}

static LRESULT CALLBACK
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

static void
Win32ReleaseFont()
{
    if (FontFace)
    {
        FontFace->Release();
        FontFace = NULL;
    }
}

#endif
