#pragma once

#include "cim_private.h"

#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>

#pragma comment (lib, "d3dcompiler")
#pragma comment (lib, "dxguid.lib")

// ============================================================
// ============================================================
// DX11 TYPE DEFINITIONS FOR CIM. BY SECTION.
// ============================================================
// ============================================================

#define CimDx11_Release(obj) if(obj) obj->lpVtbl->Release(obj); obj = NULL;
#define Cim_AssertHR(hr) Cim_Assert((SUCCEEDED(hr)));

typedef struct cim_dx11_backend
{
    ID3D11Device        *Device;
    ID3D11DeviceContext *DeviceContext;

    ID3D11Buffer *VtxBuffer;
    ID3D11Buffer *IdxBuffer;
    UINT          VtxBufferSize;
    UINT          IdxBufferSize;
} cim_dx11_backend;

#ifdef __cplusplus
extern "C" {
#endif

void CimDx11_Initialize(ID3D11Device *UserDevice, ID3D11DeviceContext *UserContext);
void CimDx11_RenderUI();

#ifdef __cplusplus
}
#endif
