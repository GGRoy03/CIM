#pragma once

#include "cim_private.h"

#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>

// NOTE: Those should be linked by the user already?
#pragma comment (lib, "dxgi")
#pragma comment (lib, "d3d11")
#pragma comment (lib, "d3dcompiler")
#pragma comment(lib, "dxguid.lib")

// ============================================================
// ============================================================
// DX11 TYPE DEFINITIONS FOR CIM. BY SECTION.
// -[SECTION]: Hashing
// ============================================================
// ============================================================

// -[SECTION:Hashing] {

#define Dx11MapBucketGroupSize 16
#define Dx11MapEmptyBucketTag  0x80

typedef struct cim_dx11_group
{
	ID3D11Buffer *VtxBuffer;
	ID3D11Buffer *IdxBuffer;
	ID3D11Buffer *VertexConstants;

	ID3D11VertexShader *VtxShader;
	ID3D11PixelShader  *PxlShader;
	ID3D11InputLayout  *Layout;
	UINT                Stride;
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

// -[SECTION:Material] {

// } -[SECTION:Materia]

typedef struct cim_dx11_backend
{
	ID3D11Device        *Device;
	ID3D11DeviceContext *DeviceContext;

	cim_dx11_group_map GroupMap;
} cim_dx11_backend;

#define Cim_AssertHR(hr) Cim_Assert((SUCCEEDED(hr)));

void CimDx11_Initialize(ID3D11Device *UserDevice, ID3D11DeviceContext *UserContext);