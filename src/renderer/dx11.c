#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>

#pragma comment (lib, "d3dcompiler")
#pragma comment (lib, "dxguid.lib")

#include "private/cim_private.h"
#include "public/cim_public.h"

#define CimDx11_Release(obj) if(obj) obj->lpVtbl->Release(obj); obj = NULL;
#define Cim_AssertHR(hr) Cim_Assert((SUCCEEDED(hr)));

typedef struct cim_dx11_pipeline
{
    ID3D11InputLayout *Layout;
    ID3D11VertexShader *VtxShader;
    ID3D11PixelShader *PxlShader;

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
    cim_u8 *Metadata;
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
    ID3D11Device *Device;
    ID3D11DeviceContext *DeviceContext;

    ID3D11Buffer *VtxBuffer;
    ID3D11Buffer *IdxBuffer;
    UINT          VtxBufferSize;
    UINT          IdxBufferSize;

    ID3D11Buffer *SharedFrameData;

    cim_dx11_pipeline_hashmap PipelineStore;
} cim_dx11_backend;

static const char *CimDx11_VertexShader =
"cbuffer PerFrame : register(b0)                                        \n"
"{                                                                      \n"
"    matrix SpaceMatrix;                                                \n"
"};                                                                     \n"
"                                                                       \n"
"struct VertexInput                                                     \n"
"{                                                                      \n"
"    float2 Pos : POSITION;                                             \n"
"    float2 Tex : NORMAL;                                               \n"
"    float4 Col : COLOR;                                                \n"
"};                                                                     \n"
"                                                                       \n"
"struct VertexOutput                                                    \n"
"{                                                                      \n"
"   float4 Position : SV_POSITION;                                      \n"
"   float4 Col      : COLOR;                                            \n"
"};                                                                     \n"
"                                                                       \n"
"VertexOutput VSMain(VertexInput Input)                                 \n"
"{                                                                      \n"
"    VertexOutput Output;                                               \n"
"                                                                       \n"
"    Output.Position = mul(SpaceMatrix, float4(Input.Pos, 1.0f, 1.0f)); \n"
"    Output.Col      = Input.Col;                                       \n"
"                                                                       \n"
"    return Output;                                                     \n"
"}                                                                      \n"
;

static const char *CimDx11_PixelShader =
"struct PSInput                           \n"
"{                                        \n"
"    float4 Position : SV_POSITION;       \n"
"    float4 Col      : COLOR;             \n"
"};                                       \n"
"                                         \n"
"float4 PSMain(PSInput Input) : SV_TARGET \n"
"{                                        \n"
"    float4 Color = Input.Col;            \n"
"    return Color;                        \n"
"}                                        \n"
;


#ifdef __cplusplus
extern "C" {
#endif

static ID3DBlob *
CimDx11_CompileShader(const char       *ByteCode,
                      size_t            ByteCodeSize,
                      const char       *EntryPoint,
                      const char       *Profile,
                      D3D_SHADER_MACRO *Defines,
                      UINT              Flags)
{
    ID3DBlob *ShaderBlob = NULL;
    ID3DBlob *ErrorBlob  = NULL;

    HRESULT Status = D3DCompile(ByteCode, ByteCodeSize,
                                NULL, Defines, NULL,
                                EntryPoint, Profile, Flags, 0,
                                &ShaderBlob, &ErrorBlob);
    Cim_AssertHR(Status);

    return ShaderBlob;
}

static ID3D11VertexShader *
CimDx11_CreateVtxShader(D3D_SHADER_MACRO *Defines,
                        ID3DBlob        **OutShaderBlob)
{
    HRESULT           Status  = S_OK;
    cim_context      *Ctx     = CimContext;
    cim_dx11_backend *Backend = Ctx->Backend;

    const char  *EntryPoint   = "VSMain";
    const char  *Profile      = "vs_5_0";
    const char  *ByteCode     = CimDx11_VertexShader;
    const SIZE_T ByteCodeSize = strlen(CimDx11_VertexShader);

    UINT CompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

    *OutShaderBlob = CimDx11_CompileShader(ByteCode, ByteCodeSize,
                                           EntryPoint, Profile,
                                           Defines, CompileFlags);

    ID3D11VertexShader *VertexShader = NULL;
    Status = ID3D11Device_CreateVertexShader(Backend->Device, ID3D10Blob_GetBufferPointer(*OutShaderBlob),
                                             ID3D10Blob_GetBufferSize(*OutShaderBlob), NULL, &VertexShader);
    Cim_AssertHR(Status);

    return VertexShader;
}

static ID3D11PixelShader *
CimDx11_CreatePxlShader(D3D_SHADER_MACRO *Defines)
{
    HRESULT           Status  = S_OK;
    ID3DBlob         *Blob    = NULL;
    cim_context      *Ctx     = CimContext;
    cim_dx11_backend *Backend = Ctx->Backend;

    const char  *EntryPoint   = "PSMain";
    const char  *Profile      = "ps_5_0";
    const char  *ByteCode     = CimDx11_PixelShader;
    const SIZE_T ByteCodeSize = strlen(CimDx11_PixelShader);

    UINT CompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

    Blob = CimDx11_CompileShader(ByteCode, ByteCodeSize,
                                 EntryPoint, Profile,
                                 Defines, CompileFlags);

    ID3D11PixelShader *PixelShader = NULL;
    Status = ID3D11Device_CreatePixelShader(Backend->Device,
                                            ID3D10Blob_GetBufferPointer(Blob),
                                            ID3D10Blob_GetBufferSize(Blob),
                                            NULL, &PixelShader);
    Cim_AssertHR(Status);

    return PixelShader;
}

static UINT
CimDx11_GetFormatSize(DXGI_FORMAT Format)
{
    switch(Format)
    {

    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
        return 4;

    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        return 8;

    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_TYPELESS:
        return 12;

    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        return 16;

    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_D16_UNORM:
        return 2;

    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16_SINT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_TYPELESS:
        return 4;

    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        return 8;

    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_TYPELESS:
        return 1;

    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SINT:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
        return 2;

    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_R10G10B10A2_UINT:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R11G11B10_FLOAT:
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
        return 4;

    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
    case DXGI_FORMAT_B5G6R5_UNORM:
    case DXGI_FORMAT_B5G5R5A1_UNORM:
        return 2;

    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        return 4;

    default:
        return 0;

    }
}

static ID3D11InputLayout *
CimDx11_CreateInputLayout(cim_bit_field Features,
                          ID3DBlob     *VtxBlob,
                          UINT         *OutStride)
{
    ID3D11ShaderReflection *Reflection = NULL;
    D3DReflect(ID3D10Blob_GetBufferPointer(VtxBlob), ID3D10Blob_GetBufferSize(VtxBlob),
               &IID_ID3D11ShaderReflection, &Reflection);

    D3D11_SHADER_DESC ShaderDesc;
    Reflection->lpVtbl->GetDesc(Reflection, &ShaderDesc);

    D3D11_INPUT_ELEMENT_DESC Desc[32] = {0};
    UINT                     Offset   = 0;
    for (cim_u32 InputIdx = 0; InputIdx < ShaderDesc.InputParameters; InputIdx++)
    {
        D3D11_SIGNATURE_PARAMETER_DESC Signature;
        Reflection->lpVtbl->GetInputParameterDesc(Reflection, InputIdx, &Signature);

        D3D11_INPUT_ELEMENT_DESC *InputDesc = Desc + InputIdx;
        InputDesc->SemanticName   = Signature.SemanticName;
        InputDesc->SemanticIndex  = Signature.SemanticIndex;
        InputDesc->InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

        if (Signature.Mask == 1)
        {
            if      (Signature.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                InputDesc->Format = DXGI_FORMAT_R32_UINT;
            else if (Signature.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                InputDesc->Format = DXGI_FORMAT_R32_SINT;
            else if (Signature.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                InputDesc->Format = DXGI_FORMAT_R32_FLOAT;
        }
        else if (Signature.Mask <= 3)
        {
            if      (Signature.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                InputDesc->Format = DXGI_FORMAT_R32G32_UINT;
            else if (Signature.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                InputDesc->Format = DXGI_FORMAT_R32G32_SINT;
            else if (Signature.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                InputDesc->Format = DXGI_FORMAT_R32G32_FLOAT;
        }
        else if (Signature.Mask <= 7)
        {
            if      (Signature.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                InputDesc->Format = DXGI_FORMAT_R32G32B32_UINT;
            else if (Signature.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                InputDesc->Format = DXGI_FORMAT_R32G32B32_SINT;
            else if (Signature.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                InputDesc->Format = DXGI_FORMAT_R32G32B32_FLOAT;
        }
        else if (Signature.Mask <= 15)
        {
            if      (Signature.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                InputDesc->Format = DXGI_FORMAT_R32G32B32A32_UINT;
            else if (Signature.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                InputDesc->Format = DXGI_FORMAT_R32G32B32A32_SINT;
            else if (Signature.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                InputDesc->Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        }

        InputDesc->AlignedByteOffset = Offset;
        Offset                      += CimDx11_GetFormatSize(InputDesc->Format);
    }

    HRESULT            Status = S_OK;
    ID3D11InputLayout *Layout = NULL;
    cim_dx11_backend *Backend = (cim_dx11_backend *)CimContext->Backend;

    Status = ID3D11Device_CreateInputLayout(Backend->Device, Desc,
                                            ShaderDesc.InputParameters,
                                            ID3D10Blob_GetBufferPointer(VtxBlob),
                                            ID3D10Blob_GetBufferSize(VtxBlob),
                                            &Layout);
    Cim_AssertHR(Status);

    *OutStride = Offset;

    return Layout;
}

// } -[SECTION:Shaders]

void CimDx11_Initialize(ID3D11Device        *UserDevice,
                        ID3D11DeviceContext *UserContext)
{
    // NOTE: Not really sure where we want to initialize this.
    CimContext = malloc(sizeof(cim_context)); Cim_Assert(CimContext);
    memset(CimContext, 0, sizeof(cim_context));

    cim_context      *Ctx     = CimContext;
    cim_dx11_backend *Backend = malloc(sizeof(cim_dx11_backend));

    if (!Backend)
    {
        return;
    }
    else
    {
        memset(Backend, 0, sizeof(cim_dx11_backend));
    }

    Backend->Device        = UserDevice;
    Backend->DeviceContext = UserContext;

    Ctx->Backend = Backend;

    // NOTE: Hijacking this function for simplicty.
    // CimStyle_ParseFile("D:/Work/CIM/styles/window.cim");
}

// [SECTION:Pipeline] {

static cim_dx11_pipeline
CimDx11_CreatePipeline(cim_bit_field Features)
{
    cim_dx11_pipeline Pipeline = {0};

    cim_u32          Enabled                       = 0;
    D3D_SHADER_MACRO Defines[CimFeature_Count + 1] = {0};

    if (Features & CimFeature_AlbedoMap)
    {
        Defines[Enabled++] = (D3D_SHADER_MACRO){"HAS_ALBEDO_MAP", "1"};
    }

    if (Features & CimFeature_MetallicMap)
    {
        Defines[Enabled++] = (D3D_SHADER_MACRO){ "HAS_METALLIC_MAP", "1" };
    }

    ID3DBlob *VSBlob = NULL;
    Pipeline.VtxShader = CimDx11_CreateVtxShader(Defines, &VSBlob); Cim_Assert(VSBlob);
    Pipeline.PxlShader = CimDx11_CreatePxlShader(Defines);
    Pipeline.Layout = CimDx11_CreateInputLayout(Features, VSBlob, &Pipeline.Stride);

    CimDx11_Release(VSBlob);

    return Pipeline;
}

static cim_dx11_pipeline *
CimDx11_GetOrCreatePipeline(cim_bit_field              Key,
                            cim_dx11_pipeline_hashmap *Hashmap)
{
    if (!Hashmap->IsInitialized)
    {
        Hashmap->GroupCount = 32;

        cim_u32 BucketCount = Hashmap->GroupCount * CimBucketGroupSize;

        Hashmap->Buckets = malloc(BucketCount * sizeof(cim_dx11_entry));
        Hashmap->Metadata = malloc(BucketCount * sizeof(cim_u8));

        if (!Hashmap->Buckets || !Hashmap->Metadata)
        {
            return NULL;
        }

        memset(Hashmap->Buckets, 0, BucketCount * sizeof(cim_dx11_entry));
        memset(Hashmap->Metadata, CimEmptyBucketTag, BucketCount * sizeof(cim_u8));

        Hashmap->IsInitialized = true;
    }

    cim_u32 ProbeCount  = 0;
    cim_u32 HashedValue = CimHash_Block32(&Key, sizeof(Key));
    cim_u32 GroupIndex  = HashedValue & (Hashmap->GroupCount - 1);

    while (true)
    {
        cim_u8 *Meta = Hashmap->Metadata + (GroupIndex * CimBucketGroupSize);
        cim_u8  Tag = (HashedValue & 0x7F);

        __m128i MetaVector = _mm_loadu_si128((__m128i *)Meta);
        __m128i TagVector = _mm_set1_epi8(Tag);

        cim_i32 Mask = _mm_movemask_epi8(_mm_cmpeq_epi8(MetaVector, TagVector));

        while (Mask)
        {
            cim_u32 Lane = CimHash_FindFirstBit32(Mask);
            cim_u32 Index = (GroupIndex * CimBucketGroupSize) + Lane;

            cim_dx11_entry *Entry = Hashmap->Buckets + Index;
            if (Entry->Key == Key)
            {
                return &Entry->Value;
            }

            Mask &= Mask - 1;
        }

        __m128i EmptyVector = _mm_set1_epi8(CimEmptyBucketTag);
        cim_i32 MaskEmpty   = _mm_movemask_epi8(_mm_cmpeq_epi8(MetaVector, EmptyVector));

        if (MaskEmpty)
        {
            cim_u32 Lane  = CimHash_FindFirstBit32(MaskEmpty);
            cim_u32 Index = (GroupIndex * CimBucketGroupSize) + Lane;

            cim_dx11_entry *Entry = Hashmap->Buckets + Index;
            Entry->Key   = Key;
            Entry->Value = CimDx11_CreatePipeline(Key);

            Meta[Lane] = Tag;

            return &Entry->Value;
        }

        ProbeCount++;
        GroupIndex = (GroupIndex + (ProbeCount * ProbeCount)) & (Hashmap->GroupCount - 1);
    }
}

// } [SECTION:Pipeline]

// [SECTION:Commands] {

// TODO:
// Implement the textures: Binding, Creation, Updating, Uber-Shader

void
CimDx11_SetupRenderState(cim_i32           ClientWidth,
                         cim_i32           ClientHeight,
                         cim_dx11_backend *Backend)
{
    ID3D11Device        *Device    = Backend->Device;
    ID3D11DeviceContext *DeviceCtx = Backend->DeviceContext;

    if (!Backend->SharedFrameData)
    {
        D3D11_BUFFER_DESC Desc = { 0 };
        Desc.ByteWidth      = sizeof(cim_dx11_shared_data);
        Desc.Usage          = D3D11_USAGE_DYNAMIC;
        Desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
        Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        Cim_AssertHR(ID3D11Device_CreateBuffer(Device, &Desc, NULL, &Backend->SharedFrameData));
    }

    D3D11_MAPPED_SUBRESOURCE MappedResource;
    if (ID3D11DeviceContext_Map(DeviceCtx, Backend->SharedFrameData, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource) == S_OK)
    {
        cim_dx11_shared_data *SharedData = (cim_dx11_shared_data * )MappedResource.pData;

        cim_f32 Left  = 0;
        cim_f32 Right = ClientWidth;
        cim_f32 Top   = 0;
        cim_f32 Bot   = ClientHeight;

        cim_f32 SpaceMatrix[4][4] =
        {
            { 2.0f / (Right - Left)          , 0.0f                     , 0.0f, 0.0f },
            { 0.0f                           , 2.0f / (Top - Bot)       , 0.0f, 0.0f },
            { 0.0f                           , 0.0f                     , 0.5f, 0.0f },
            { (Right + Left) / (Left - Right), (Top + Bot) / (Bot - Top), 0.5f, 1.0f },
        };

        memcpy(&SharedData->SpaceMatrix, SpaceMatrix, sizeof(SpaceMatrix));
        ID3D11DeviceContext_Unmap(DeviceCtx, Backend->SharedFrameData, 0);
    }

    ID3D11DeviceContext_IASetPrimitiveTopology(DeviceCtx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_IASetIndexBuffer(DeviceCtx, Backend->IdxBuffer, DXGI_FORMAT_R32_UINT, 0);
    ID3D11DeviceContext_VSSetConstantBuffers(DeviceCtx, 0, 1, &Backend->SharedFrameData);

    D3D11_VIEWPORT Viewport;
    Viewport.TopLeftX = 0.0f;
    Viewport.TopLeftY = 0.0f;
    Viewport.Width = ClientWidth;
    Viewport.Height = ClientHeight;
    Viewport.MinDepth = 0.0f;
    Viewport.MaxDepth = 1.0f;
    ID3D11DeviceContext_RSSetViewports(DeviceCtx, 1, &Viewport);
}

void 
CimDx11_RenderUI(cim_i32 ClientWidth,
                 cim_i32 ClientHeight)
{
    cim_context      *Ctx     = CimContext;                      Cim_Assert(Ctx);
    cim_dx11_backend *Backend = (cim_dx11_backend*)Ctx->Backend; Cim_Assert(Backend);

    if (ClientWidth == 0 || ClientHeight == 0)
    {
        return;
    }

    // Mmmm, not sure.

    HRESULT              Status    = S_OK;
    ID3D11Device        *Device    = Backend->Device;        Cim_Assert(Device);
    ID3D11DeviceContext *DeviceCtx = Backend->DeviceContext; Cim_Assert(DeviceCtx);
    cim_command_buffer  *CmdBuffer = &Ctx->CmdBuffer;

    // NOTE: These should either be called by the user or fed as inputs.
    CimCommand_BuildDrawData(CmdBuffer);
    CimConstraint_Solve(&Ctx->Inputs);

    if (!Backend->VtxBuffer || CmdBuffer->FrameVtx.At > Backend->VtxBufferSize)
    {
        CimDx11_Release(Backend->VtxBuffer);

        Backend->VtxBufferSize = CmdBuffer->FrameVtx.At + 1024;

        D3D11_BUFFER_DESC Desc = { 0 };
        Desc.ByteWidth      = Backend->VtxBufferSize;
        Desc.Usage          = D3D11_USAGE_DYNAMIC;
        Desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
        Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        Status = ID3D11Device_CreateBuffer(Device, &Desc, NULL, &Backend->VtxBuffer); Cim_AssertHR(Status);
    }

    if (!Backend->IdxBuffer || CmdBuffer->FrameIdx.At > Backend->IdxBufferSize)
    {
        CimDx11_Release(Backend->IdxBuffer);

        Backend->IdxBufferSize = CmdBuffer->FrameIdx.At + 1024;

        D3D11_BUFFER_DESC Desc = { 0 };
        Desc.ByteWidth      = Backend->IdxBufferSize;
        Desc.Usage          = D3D11_USAGE_DYNAMIC;
        Desc.BindFlags      = D3D11_BIND_INDEX_BUFFER;
        Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        Status = ID3D11Device_CreateBuffer(Device, &Desc, NULL, &Backend->IdxBuffer); Cim_AssertHR(Status);
    }

    D3D11_MAPPED_SUBRESOURCE VtxResource = { 0 };
    Status = ID3D11DeviceContext_Map(DeviceCtx, Backend->VtxBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &VtxResource); Cim_AssertHR(Status);
    if (FAILED(Status) || !VtxResource.pData)
    {
        return;
    }
    memcpy(VtxResource.pData, CmdBuffer->FrameVtx.Memory, CmdBuffer->FrameVtx.At);
    ID3D11DeviceContext_Unmap(DeviceCtx, Backend->VtxBuffer, 0);

    D3D11_MAPPED_SUBRESOURCE IdxResource = { 0 };
    Status = ID3D11DeviceContext_Map(DeviceCtx, Backend->IdxBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &IdxResource); Cim_AssertHR(Status);
    if (FAILED(Status) || !IdxResource.pData)
    {
        return;
    }
    memcpy(IdxResource.pData, CmdBuffer->FrameIdx.Memory, CmdBuffer->FrameIdx.At);
    ID3D11DeviceContext_Unmap(DeviceCtx, Backend->IdxBuffer, 0);

    // ===============================================================================

    CimDx11_SetupRenderState(ClientWidth, ClientHeight, Backend);

    // ===============================================================================
    cim_u32 CmdCount = Ctx->CmdBuffer.Commands.WriteOffset;
    for(cim_u32 CmdIdx = 0; CmdIdx < CmdCount; CmdIdx++)
    {
        cim_draw_command  *Command  = CimCommandStream_Read(1, &Ctx->CmdBuffer.Commands); 
        cim_dx11_pipeline *Pipeline = CimDx11_GetOrCreatePipeline(Command->Features, &Backend->PipelineStore);

        Cim_Assert(Command && Pipeline);

        ID3D11DeviceContext_IASetInputLayout(DeviceCtx, Pipeline->Layout);
        ID3D11DeviceContext_IASetVertexBuffers(DeviceCtx, 0, 1, &Backend->VtxBuffer, &Pipeline->Stride, &Pipeline->Offset);

        ID3D11DeviceContext_VSSetShader(DeviceCtx, Pipeline->VtxShader, NULL, NULL);

        ID3D11DeviceContext_PSSetShader(DeviceCtx, Pipeline->PxlShader, NULL, NULL);

        ID3D11DeviceContext_DrawIndexed(DeviceCtx, Command->IdxCount, Command->IdxOffset, Command->VtxOffset);
    }

    // NOTE: This is not pretty. We have to reset some way right...
    CimArena_Reset(&CmdBuffer->FrameVtx);
    CimArena_Reset(&CmdBuffer->FrameIdx);
    CimArena_Reset(&CmdBuffer->Batches);
    CimCommandStream_Reset(&CmdBuffer->Commands);
    CimQuadStream_Reset(&CmdBuffer->Quads);
}

// } [SECTION:Commands]

#ifdef __cplusplus
}
#endif
