#pragma once

#include "private/cim_private.h"

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
// -[SECTION:Pipeline]
// ============================================================
// ============================================================

#define CimDx11_Release(obj) if(obj) obj->lpVtbl->Release(obj); obj = NULL;
#define Cim_AssertHR(hr) Cim_Assert((SUCCEEDED(hr)));

// [SECTION:Pipeline] {

typedef struct cim_dx11_pipeline
{
    ID3D11InputLayout  *Layout;
    ID3D11VertexShader *VtxShader;
    ID3D11PixelShader  *PxlShader;

    UINT Stride;
    UINT Offset;
} cim_dx11_pipeline;

typedef struct cim_dx11_entry
{
    cim_bit_field     Key;
    cim_dx11_pipeline Value;
} cim_dx11_entry;

typedef struct cim_dx11_pipeline_hashmap
{
    cim_u8         *Metadata;
    cim_dx11_entry *Buckets;
    cim_u32         GroupCount;
    bool            IsInitialized;
} cim_dx11_pipeline_hashmap;

// } [SECTION:Pipeline]

typedef struct cim_dx11_shared_data
{
    cim_f32 SpaceMatrix[4][4];
} cim_dx11_shared_data;

typedef struct cim_dx11_backend
{
    ID3D11Device        *Device;
    ID3D11DeviceContext *DeviceContext;

    ID3D11Buffer *VtxBuffer;
    ID3D11Buffer *IdxBuffer;
    UINT          VtxBufferSize;
    UINT          IdxBufferSize;

    ID3D11Buffer *SharedFrameData;

    cim_dx11_pipeline_hashmap PipelineStore;
} cim_dx11_backend;

#ifdef __cplusplus
extern "C" {
#endif

void CimDx11_Initialize(ID3D11Device *UserDevice, ID3D11DeviceContext *UserContext);
void CimDx11_RenderUI(cim_i32 ClientWidth, cim_i32 ClientHeight);

#ifdef __cplusplus
}
#endif
