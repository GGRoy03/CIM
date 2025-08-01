#include <stdio.h> // For printf

#include "platform/cim_win32.c"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <d3d11.h>
#include <dxgi.h>

#pragma comment (lib, "dxgi")
#pragma comment (lib, "d3d11")

#define Assert(cond) do { if (!(cond)) __debugbreak(); } while (0)
#define AssertHR(Status) do { if (!(SUCCEEDED(Status))) __debugbreak(); } while (0)

struct win32_window
{
    HWND Handle;
    int  Width;
    int  Height;
};

struct dx11_context
{
    ID3D11Device           *Device;
    ID3D11DeviceContext    *DeviceContext;
    IDXGISwapChain         *SwapChain;
    ID3D11RenderTargetView *RenderView;
};

static LRESULT CALLBACK
Win32_WindowProc(HWND Handle, UINT Message, WPARAM WParam, LPARAM LParam)
{
    if (CimWin32_WindowProc(Handle, Message, WParam, LParam, NULL))
    {
        return 0;
    }

    switch(Message)
    {

    default:
    {
        return DefWindowProcW(Handle, Message, WParam, LParam);
    } break;

    }
}

static void 
Win32_GetClientSize(HWND Handle, int *OutWidth, int *OutHeight)
{
    RECT Rect;
    GetClientRect(Handle, &Rect);
    *OutWidth  = Rect.right - Rect.left;
    *OutHeight = Rect.bottom - Rect.top;
}

static HMODULE 
Win32_GetModuleHandle()
{
    HMODULE ImageBase;
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCWSTR)&Win32_GetModuleHandle, &ImageBase)) 
    {
        return ImageBase;
    }

    return 0;
}

static win32_window
Win32_Initialize(int Width, int Height)
{
    win32_window Window = {};

    WNDCLASSEXW WindowClass = {};
    WindowClass.cbSize        = sizeof(WindowClass);
    WindowClass.lpfnWndProc   = Win32_WindowProc;
    WindowClass.hInstance     = (HINSTANCE)Win32_GetModuleHandle();
    WindowClass.hIcon         = LoadIcon(nullptr, IDI_APPLICATION);
    WindowClass.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    WindowClass.lpszClassName = L"cim_test_win32_window";

    RegisterClassExW(&WindowClass);

    DWORD EXStyle = WS_EX_APPWINDOW;
    DWORD Style   = WS_OVERLAPPEDWINDOW;

    Window.Handle = CreateWindowExW(EXStyle, WindowClass.lpszClassName, L"Window",
                                    Style, CW_USEDEFAULT, CW_USEDEFAULT, Width, Height,
                                    NULL, NULL, WindowClass.hInstance, NULL);

    ShowWindow(Window.Handle, SW_SHOWDEFAULT);
    Win32_GetClientSize(Window.Handle, &Window.Width, &Window.Height);

    return Window;
};

static bool
Win32_ProcessMessages()
{
    MSG Message;
    while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch (Message.message)
        {

        case WM_QUIT:
        case WM_DESTROY:
        {
            return false;
        } break;

        default:
        {
            TranslateMessage(&Message);
            DispatchMessageA(&Message);
        } break;
        }
    }

    return true;
}

static dx11_context
Dx11_Initialize(win32_window Window)
{
    dx11_context Dx11   = {};
    HRESULT      Status = S_OK;

    DXGI_SWAP_CHAIN_DESC SDesc = {};
    SDesc.BufferCount       = 2;
    SDesc.BufferDesc.Width  = Window.Width;
    SDesc.BufferDesc.Height = Window.Height;
    SDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    SDesc.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SDesc.OutputWindow      = Window.Handle;
    SDesc.SampleDesc.Count  = 1;
    SDesc.Windowed          = TRUE;
    SDesc.SwapEffect        = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    UINT CreateFlags = D3D11_CREATE_DEVICE_DEBUG;
    Status =  D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                                            CreateFlags, nullptr, 0, D3D11_SDK_VERSION,
                                            &SDesc, &Dx11.SwapChain, &Dx11.Device,
                                            nullptr, &Dx11.DeviceContext); 
    AssertHR(Status);

    ID3D11Texture2D* BackBuffer = nullptr;
    Status = Dx11.SwapChain->GetBuffer(0, IID_PPV_ARGS(&BackBuffer));                    AssertHR(Status);
    Status = Dx11.Device->CreateRenderTargetView(BackBuffer, nullptr, &Dx11.RenderView); AssertHR(Status);

    BackBuffer->Release();

    return Dx11;
}

int main()
{
    win32_window Win32 = Win32_Initialize(1920, 1080);
    dx11_context Dx11  = Dx11_Initialize(Win32);

    CimDx11_Initialize(Dx11.Device, Dx11.DeviceContext);

    while(Win32_ProcessMessages())
    {
        Dx11.DeviceContext->OMSetRenderTargets(1, &Dx11.RenderView, nullptr);

        const FLOAT ClearColor[4] = {0.2f, 0.3f, 0.4f, 1.0f};
        Dx11.DeviceContext->ClearRenderTargetView(Dx11.RenderView, ClearColor);

        cim_vector4 WindowColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        if(Cim_Window("Test Window", WindowColor, CimWindow_Draggable))
        {
        }
        else
        {
        }

        Win32_GetClientSize(Win32.Handle, &Win32.Width, &Win32.Height);
        CimDx11_RenderUI(Win32.Width, Win32.Height);

        // Cim_EndFrame();

        Dx11.SwapChain->Present(1, 0);
    }
}
