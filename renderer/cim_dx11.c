#include "cim_dx11.h"
#include "cim_private.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ============================================================
// DX11 IMPLEMENTATION FOR CIM. BY SECTION.
// -[SECTION] Shaders
// -[SECTION] Pipelines
// -[SECTION] Commands
// ============================================================
// ============================================================

// -[SECTION:Shaders] {
// TODO: 
// 1) Make the shader compilation flags user based.

// WARN:
// 1) When creating a layout InstanceDataStepRate and InputSlot are always 0.

static ID3DBlob *
CimDx11_CompileShader(const char *ByteCode, size_t ByteCodeSize, const char *EntryPoint, const char *Profile, D3D_SHADER_MACRO *Defines, UINT Flags)
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
CimDx11_CreateVtxShader(D3D_SHADER_MACRO *Defines, ID3DBlob **OutShaderBlob)
{
    HRESULT           Status  = S_OK;
    cim_context      *Ctx     = CimContext;
    cim_dx11_backend *Backend = Ctx->Backend;

    const char  *EntryPoint   = "VSMain";
    const char  *Profile      = "vs_5_0";
    const char  *ByteCode     = "";
    const SIZE_T ByteCodeSize = 0;

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

    const char  *EntryPoint   = "VSMain";
    const char  *Profile      = "vs_5_0";
    const char  *ByteCode     = "";
    const SIZE_T ByteCodeSize = 0;

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

// CREDITS: Chad Gee-Pee-Tee

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
CimDx11_CreateInputLayout(cim_bit_field Features, ID3DBlob *VtxBlob, UINT *OutStride)
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

void CimDx11_Initialize(ID3D11Device *UserDevice, ID3D11DeviceContext *UserContext)
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

    Backend->Device        = UserDevice;
    Backend->DeviceContext = UserContext;

    Ctx->Backend = Backend;
}

// [SECTION:Pipeline] {
// BUG:
// 1) We are missing a hash for our 32 bits integer values, defaulting to 0.

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
        Defines[Enabled++] = (D3D_SHADER_MACRO){"HAS_METALLIC_MAP", "1"};
    }

    ID3DBlob *VSBlob   = NULL;
    Pipeline.VtxShader = CimDx11_CreateVtxShader(Defines, &VSBlob); Cim_Assert(VSBlob);
    Pipeline.PxlShader = CimDx11_CreatePxlShader(Defines);
    Pipeline.Layout    = CimDx11_CreateInputLayout(Features, VSBlob, &Pipeline.Stride);

    return Pipeline;
}

static cim_dx11_pipeline *
CimDx11_GetOrCreatePipeline(cim_bit_field Key, cim_dx11_pipeline_hashmap *Hashmap)
{
    if(!Hashmap->IsInitialized)
    {
        Hashmap->GroupCount = 32;

        cim_u32 BucketCount = Hashmap->GroupCount * CimBucketGroupSize;

        Hashmap->Buckets  = malloc(BucketCount * sizeof(cim_dx11_entry));
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
    cim_u32 HashedValue = 0;
    cim_u32 GroupIndex  = HashedValue & (Hashmap->GroupCount - 1);

    while (true)
    {
        cim_u8 *Meta = Hashmap->Metadata + (GroupIndex * CimBucketGroupSize);
        cim_u8  Tag  = (HashedValue & 0x7F);

        __m128i MetaVector = _mm_loadu_si128((__m128i *)Meta);
        __m128i TagVector  = _mm_set1_epi8(Tag);

        cim_i32 Mask = _mm_movemask_epi8(_mm_cmpeq_epi8(MetaVector, TagVector));

        while (Mask)
        {
            cim_u32 Lane  = Cim_FindFirstBit32(Mask);
            cim_u32 Index = (GroupIndex * CimBucketGroupSize) + Lane;

            cim_dx11_entry *Entry = Hashmap->Buckets + Index;
            if(Entry->Key == Key)
            {
                return &Entry->Value;
            }

            Mask &= Mask - 1;
        }

        __m128i EmptyVector = _mm_set1_epi8(CimEmptyBucketTag);
        cim_i32 MaskEmpty   = _mm_movemask_epi8(_mm_cmpeq_epi8(MetaVector, EmptyVector));

        if (MaskEmpty)
        {
            cim_u32 Lane  = Cim_FindFirstBit32(MaskEmpty);
            cim_u32 Index = (GroupIndex * CimBucketGroupSize) + Lane;

            cim_dx11_entry *Entry = Hashmap->Buckets + Index;
            Entry->Key   = Key;
            Entry->Value = CimDx11_CreatePipeline(Key);

            return &Entry->Value;
        }

        ProbeCount++;
        GroupIndex = (GroupIndex + (ProbeCount * ProbeCount)) & (Hashmap->GroupCount - 1);
    }
}

// } [SECTION:Pipeline]

// [SECTION:Commands] {

// WARN:
// 1) Should probably crash on malloc failure?

// TODO:
// Implement the textures: Binding, Creation, Updating, Uber-Shader

void CimDx11_RenderUI()
{
    cim_context      *Ctx     = CimContext;                       Cim_Assert(Ctx);
    cim_dx11_backend *Backend = (cim_dx11_backend*)&Ctx->Backend; Cim_Assert(Backend);

    HRESULT              Status    = S_OK;
    ID3D11Device        *Device    = Backend->Device;        Cim_Assert(Device);
    ID3D11DeviceContext *DeviceCtx = Backend->DeviceContext; Cim_Assert(DeviceCtx);

    cim_command_buffer *CmdBuffer = &Ctx->CmdBuffer; Cim_Assert(CmdBuffer);

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

    if (!Backend->IdxBuffer || CmdBuffer->FrameIdx.At > Backend->VtxBufferSize)
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

    D3D11_MAPPED_SUBRESOURCE VtxResource = { 0 };
    Status = ID3D11DeviceContext_Map(DeviceCtx, &Backend->VtxBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &VtxResource); Cim_AssertHR(Status);
    if (FAILED(Status))
    {
        return;
    }
    memcpy(VtxResource.pData, CmdBuffer->FrameVtx.Memory, CmdBuffer->FrameVtx.At);
    ID3D11DeviceContext_Unmap(DeviceCtx, &Backend->VtxBuffer, 0);

    D3D11_MAPPED_SUBRESOURCE IdxResource = { 0 };
    Status = ID3D11DeviceContext_Map(DeviceCtx, &Backend->IdxBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &IdxResource); Cim_AssertHR(Status);
    if (FAILED(Status))
    {
        return;
    }
    memcpy(IdxResource.pData, CmdBuffer->FrameIdx.Memory, CmdBuffer->FrameIdx.At);
    ID3D11DeviceContext_Unmap(DeviceCtx, &Backend->IdxBuffer, 0);
    ID3D11DeviceContext_IASetIndexBuffer(DeviceCtx, &Backend->IdxBuffer, DXGI_FORMAT_R32_UINT, 0);

    cim_u32 BatchCount = CimArena_GetCount(sizeof(cim_command_batch), &CmdBuffer->Batches);
    for(cim_u32 BatchIdx = 0; BatchIdx < BatchCount; BatchIdx++)
    {
        cim_command_batch *Batch    = (cim_command_batch*)CmdBuffer->Batches.Memory + BatchIdx;
        cim_dx11_pipeline *Pipeline = CimDx11_GetOrCreatePipeline(Batch->Features, &Backend->PipelineStore);

        Cim_Assert(Batch && Pipeline);

        ID3D11DeviceContext_IASetInputLayout(DeviceCtx, Pipeline->Layout);
        ID3D11DeviceContext_IASetVertexBuffers(DeviceCtx, 0, 1, &Backend->VtxBuffer, Pipeline->Stride, Pipeline->Offset);

        ID3D11DeviceContext_VSSetShader(DeviceCtx, Pipeline->VtxShader, NULL, NULL);

        ID3D11DeviceContext_PSSetShader(DeviceCtx, Pipeline->PxlShader, NULL, NULL);

        ID3D11DeviceContext_DrawIndexed(DeviceCtx, Batch->IdxCount, Batch->IdxOffset, Batch->VtxOffset);
    }

    CimArena_Reset(&CmdBuffer->FrameVtx);
    CimArena_Reset(&CmdBuffer->FrameIdx);
    CimArena_Reset(&CmdBuffer->Batches);
}

// } [SECTION:Commands]

#ifdef __cplusplus
}
#endif
