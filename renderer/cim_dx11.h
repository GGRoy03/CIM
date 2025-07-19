#pragma once

#include "cim_private.h"

#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>

// NOTE: Those should be linked by the user already?
#pragma comment (lib, "dxgi")
#pragma comment (lib, "d3d11")
#pragma comment (lib, "d3dcompiler")


// ============================================================
// ============================================================
// DX11 TYPE DEFINITIONS FOR CIM. BY SECTION.
// -[SECTION] Hashing
// ============================================================
// ============================================================

// -[SECTION] Hashing {

#define Dx11MapBucketGroupSize 16
#define Dx11MapEmptyBucketTag  0x80

typedef struct cim_dx11_group
{
	ID3D11Buffer *VtxBuffer;
	ID3D11Buffer *IdxBuffer;

	ID3D11VertexShader *VtxShader;
	ID3D11PixelShader  *PxlShader;
	ID3D11InputLayout  *Layout;
} cim_dx11_group;

typedef struct cim_dx11_group_entry
{
	char           Key[64];
	cim_dx11_group Value;
} cim_dx11_group_entry;

typedef struct cim_dx11_group_map
{
	cim_u8               *MetaData;
	cim_dx11_group_entry *Buckets;
	cim_u32               GroupCount;
} cim_dx11_group_map;

// } -[SECTION:Hashing]

typedef struct cim_dx11_backend
{
	ID3D11Device        *Device;
	ID3D11DeviceContext *DeviceContext;
} cim_dx11_backend;

#define AssertHR(hr) Cim_Assert((SUCCEEDED(hr)));

void CimDx11_Initialize(cim_i32 Width, cim_i32 Height, ID3D11Device *UserDevice, ID3D11DeviceContext *UserContext);