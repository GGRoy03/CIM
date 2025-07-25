#pragma once

#include "cim_private.h"

#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>

// NOTE: These should be linked by the user already?

#pragma comment (lib, "dxgi")
#pragma comment (lib, "d3d11")
#pragma comment (lib, "d3dcompiler")
#pragma comment (lib, "dxguid.lib")

// ============================================================
// ============================================================
// DX11 TYPE DEFINITIONS FOR CIM. BY SECTION.
// ============================================================
// ============================================================

#define CimDx11_Release(obj) if(obj) obj->lpVtbl->Release(obj); obj = NULL;
#define Cim_AssertHR(hr) Cim_Assert((SUCCEEDED(hr)));

typedef struct cim_dx11_batch_resource
{
    ID3D11Buffer *VtxBuffer;
    size_t        VtxBufferSize;

    ID3D11Buffer *IdxBuffer;
    size_t        IdxBufferSize;

    void    *FrameVtxData;
    cim_u32 *FrameIdxData;
} cim_dx11_batch_resource;

typedef struct cim_dx11_backend
{
    ID3D11Device        *Device;
    ID3D11DeviceContext *DeviceContext;

    cim_dx11_batch_resource BatchResources[4];
} cim_dx11_backend;

void CimDx11_Initialize(ID3D11Device *UserDevice, ID3D11DeviceContext *UserContext);
