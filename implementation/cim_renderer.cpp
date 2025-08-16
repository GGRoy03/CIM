// [Public API]

// Init functions
static void CimDx11_Initialize(void *UserDevice, void *UserContext);

// Draw functions
static void CimDx11_RenderUI(cim_i32 ClientWidth, cim_i32 ClientHeight);

static bool 
InitializeRenderer(CimRenderer_Backend Backend, void *Param1, void *Param2) // WARN: This is a bit goofy.
{
    cim_renderer *Renderer = UIP_RENDERER;

    switch (Backend)
    {

#ifdef _WIN32
    case CimRenderer_Dx11:
    {
        CimDx11_Initialize(Param1, Param2);
        Renderer->Draw = CimDx11_RenderUI;
    } break;
#endif

    default:
    {
        return false;
    } break;

    }

    return true;
}

// [Agnostic Helpers]


cim_draw_command *
GetDrawCommand(cim_cmd_buffer *Buffer)
{
    cim_draw_command *Command = NULL;

    if (Buffer->CommandCount == Buffer->CommandSize)
    {
        Buffer->CommandSize = Buffer->CommandSize ? Buffer->CommandSize * 2 : 32;

        cim_draw_command *New = (cim_draw_command *)malloc(Buffer->CommandSize * sizeof(cim_draw_command));
        if (!New)
        {
            CimLog_Fatal("Failed to heap-allocate command buffer.");
            return NULL;
        }

        if (Buffer->Commands)
        {
            memcpy(New, Buffer->Commands, Buffer->CommandCount * sizeof(cim_draw_command));
            free(Buffer->Commands);
        }

        memset(New, 0, Buffer->CommandSize * sizeof(cim_draw_command));
        Buffer->Commands = New;
    }

    if (Buffer->ClippingRectChanged || Buffer->FeatureStateChanged)
    {
        Command = Buffer->Commands + Buffer->CommandCount++;

        Command->VtxOffset = Buffer->FrameVtx.At;
        Command->IdxOffset = Buffer->FrameIdx.At;
        Command->VtxCount = 0;
        Command->IdxCount = 0;

        Buffer->ClippingRectChanged = false;
        Buffer->FeatureStateChanged = false;
    }
    else
    {
        Command = Buffer->Commands + (Buffer->CommandCount - 1);
    }

    return Command;
}

// NOTE: Have to profile which is better: Directly upload, or defer the upload
// to the renderer (More like a command type structure). Maybe we get better
// cache on the points? Have to profile.

static void
DrawQuadFromData(cim_f32 x0, cim_f32 y0, cim_f32 x1, cim_f32 y1, cim_vector4 Col)
{
    Cim_Assert(CimCurrent);

    typedef struct local_vertex
    {
        cim_f32 PosX, PosY;
        cim_f32 U, V;
        cim_f32 R, G, B, A;
    } local_vertex;

    cim_cmd_buffer   *Buffer  = UIP_COMMANDS;
    cim_draw_command *Command = GetDrawCommand(Buffer);

    local_vertex Vtx[4];
    Vtx[0] = (local_vertex){ x0, y0, 0.0f, 1.0f, Col.x, Col.y, Col.z, Col.w };
    Vtx[1] = (local_vertex){ x0, y1, 0.0f, 0.0f, Col.x, Col.y, Col.z, Col.w };
    Vtx[2] = (local_vertex){ x1, y0, 1.0f, 1.0f, Col.x, Col.y, Col.z, Col.w };
    Vtx[3] = (local_vertex){ x1, y1, 1.0f, 0.0f, Col.x, Col.y, Col.z, Col.w };

    cim_u32 Idx[6];
    Idx[0] = Command->VtxCount + 0;
    Idx[1] = Command->VtxCount + 2;
    Idx[2] = Command->VtxCount + 1;
    Idx[3] = Command->VtxCount + 2;
    Idx[4] = Command->VtxCount + 3;
    Idx[5] = Command->VtxCount + 1;

    WriteToArena(Vtx, sizeof(Vtx), &Buffer->FrameVtx);
    WriteToArena(Idx, sizeof(Idx), &Buffer->FrameIdx);;

    Command->VtxCount += 4;
    Command->IdxCount += 6;
}

static void
DrawQuadFromNode(cim_layout_node *Node, cim_vector4 Col)
{
    Cim_Assert(CimCurrent);

    typedef struct local_vertex
    {
        cim_f32 PosX, PosY;
        cim_f32 U, V;
        cim_f32 R, G, B, A;
    } local_vertex;

    cim_cmd_buffer *Buffer = UIP_COMMANDS;
    cim_draw_command *Command = GetDrawCommand(Buffer);

    cim_f32 x0 = Node->X;
    cim_f32 y0 = Node->Y;
    cim_f32 x1 = Node->X + Node->Width;
    cim_f32 y1 = Node->Y + Node->Height;

    local_vertex Vtx[4];
    Vtx[0] = (local_vertex){ x0, y0, 0.0f, 1.0f, Col.x, Col.y, Col.z, Col.w };
    Vtx[1] = (local_vertex){ x0, y1, 0.0f, 0.0f, Col.x, Col.y, Col.z, Col.w };
    Vtx[2] = (local_vertex){ x1, y0, 1.0f, 1.0f, Col.x, Col.y, Col.z, Col.w };
    Vtx[3] = (local_vertex){ x1, y1, 1.0f, 0.0f, Col.x, Col.y, Col.z, Col.w };

    cim_u32 Idx[6];
    Idx[0] = Command->VtxCount + 0;
    Idx[1] = Command->VtxCount + 2;
    Idx[2] = Command->VtxCount + 1;
    Idx[3] = Command->VtxCount + 2;
    Idx[4] = Command->VtxCount + 3;
    Idx[5] = Command->VtxCount + 1;

    WriteToArena(Vtx, sizeof(Vtx), &Buffer->FrameVtx);
    WriteToArena(Idx, sizeof(Idx), &Buffer->FrameIdx);;

    Command->VtxCount += 4;
    Command->IdxCount += 6;
}

// [Uber Shaders]

const char *Dx11VertexShader =
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

const char *Dx11PixelShader =
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

// [DX11 backend]

#ifdef _WIN32

#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>

#pragma comment (lib, "d3dcompiler")
#pragma comment (lib, "dxguid.lib")

#define CimDx11_Release(obj) if(obj) obj->Release(); obj = NULL;
#define Cim_AssertHR(hr) Cim_Assert((SUCCEEDED(hr)));

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
    size_t        VtxBufferSize;
    size_t        IdxBufferSize;

    ID3D11Buffer *SharedFrameData;

    cim_dx11_pipeline_hashmap PipelineStore;
} cim_dx11_backend;


static ID3DBlob *
CimDx11_CompileShader(const char *ByteCode, size_t ByteCodeSize, const char *EntryPoint,
                      const char *Profile, D3D_SHADER_MACRO *Defines, UINT Flags)
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
    Cim_Assert(CimCurrent);

    cim_dx11_backend *Backend = (cim_dx11_backend *)CimCurrent->Backend;
    HRESULT           Status = S_OK;

    const char *EntryPoint    = "VSMain";
    const char *Profile       = "vs_5_0";
    const char *ByteCode      = Dx11VertexShader;
    const SIZE_T ByteCodeSize = strlen(Dx11VertexShader);

    UINT CompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

    *OutShaderBlob = CimDx11_CompileShader(ByteCode, ByteCodeSize,
                                       EntryPoint, Profile,
                                       Defines, CompileFlags);

    ID3D11VertexShader *VertexShader = NULL;
    Status = Backend->Device->CreateVertexShader((*OutShaderBlob)->GetBufferPointer(), (*OutShaderBlob)->GetBufferSize(), NULL, &VertexShader);
    Cim_AssertHR(Status);

    return VertexShader;
}

static ID3D11PixelShader *
CimDx11_CreatePxlShader(D3D_SHADER_MACRO *Defines)
{
    HRESULT           Status = S_OK;
    ID3DBlob *Blob = NULL;
    cim_dx11_backend *Backend = (cim_dx11_backend *)CimCurrent->Backend;

    const char *EntryPoint    = "PSMain";
    const char *Profile       = "ps_5_0";
    const char *ByteCode      = Dx11PixelShader;
    const SIZE_T ByteCodeSize = strlen(Dx11PixelShader);

    UINT CompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

    Blob = CimDx11_CompileShader(ByteCode, ByteCodeSize,
                             EntryPoint, Profile,
                             Defines, CompileFlags);

    ID3D11PixelShader *PixelShader = NULL;
    Status = Backend->Device->CreatePixelShader(Blob->GetBufferPointer(), Blob->GetBufferSize(), NULL, &PixelShader);
    Cim_AssertHR(Status);

    return PixelShader;
}

static UINT
CimDx11_GetFormatSize(DXGI_FORMAT Format)
{
    switch (Format)
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
CimDx11_CreateInputLayout(ID3DBlob *VtxBlob, UINT *OutStride)
{
    ID3D11ShaderReflection *Reflection = NULL;
    D3DReflect(VtxBlob->GetBufferPointer(), VtxBlob->GetBufferSize(), IID_ID3D11ShaderReflection, (void **)&Reflection);

    D3D11_SHADER_DESC ShaderDesc;
    Reflection->GetDesc(&ShaderDesc);

    D3D11_INPUT_ELEMENT_DESC Desc[32] = {};
    UINT                     Offset = 0;
    for (cim_u32 InputIdx = 0; InputIdx < ShaderDesc.InputParameters; InputIdx++)
    {
        D3D11_SIGNATURE_PARAMETER_DESC Signature;
        Reflection->GetInputParameterDesc(InputIdx, &Signature);

        D3D11_INPUT_ELEMENT_DESC *InputDesc = Desc + InputIdx;
        InputDesc->SemanticName = Signature.SemanticName;
        InputDesc->SemanticIndex = Signature.SemanticIndex;
        InputDesc->InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

        if (Signature.Mask == 1)
        {
            if (Signature.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                InputDesc->Format = DXGI_FORMAT_R32_UINT;
            else if (Signature.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                InputDesc->Format = DXGI_FORMAT_R32_SINT;
            else if (Signature.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                InputDesc->Format = DXGI_FORMAT_R32_FLOAT;
        }
        else if (Signature.Mask <= 3)
        {
            if (Signature.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                InputDesc->Format = DXGI_FORMAT_R32G32_UINT;
            else if (Signature.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                InputDesc->Format = DXGI_FORMAT_R32G32_SINT;
            else if (Signature.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                InputDesc->Format = DXGI_FORMAT_R32G32_FLOAT;
        }
        else if (Signature.Mask <= 7)
        {
            if (Signature.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                InputDesc->Format = DXGI_FORMAT_R32G32B32_UINT;
            else if (Signature.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                InputDesc->Format = DXGI_FORMAT_R32G32B32_SINT;
            else if (Signature.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                InputDesc->Format = DXGI_FORMAT_R32G32B32_FLOAT;
        }
        else if (Signature.Mask <= 15)
        {
            if (Signature.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                InputDesc->Format = DXGI_FORMAT_R32G32B32A32_UINT;
            else if (Signature.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                InputDesc->Format = DXGI_FORMAT_R32G32B32A32_SINT;
            else if (Signature.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                InputDesc->Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        }

        InputDesc->AlignedByteOffset = Offset;
        Offset += CimDx11_GetFormatSize(InputDesc->Format);
    }

    HRESULT            Status = S_OK;
    ID3D11InputLayout *Layout = NULL;
    cim_dx11_backend *Backend = (cim_dx11_backend *)CimCurrent->Backend;

    Status = Backend->Device->CreateInputLayout(Desc, ShaderDesc.InputParameters,
                                                VtxBlob->GetBufferPointer(), VtxBlob->GetBufferSize(),
                                                &Layout);
    Cim_AssertHR(Status);

    *OutStride = Offset;

    return Layout;
}

static void
CimDx11_Initialize(void *UserDevice, void *UserContext)
{
    Cim_Assert(UserDevice && UserContext);

    if (!CimCurrent)
    {
        Cim_Assert(!"Context must be initialized.");
        return;
    }

    cim_dx11_backend *Backend = (cim_dx11_backend *)malloc(sizeof(cim_dx11_backend));

    if (!Backend)
    {
        return;
    }
    else
    {
        memset(Backend, 0, sizeof(cim_dx11_backend));
    }

    Backend->Device        = (ID3D11Device*)UserDevice;
    Backend->DeviceContext = (ID3D11DeviceContext*)UserContext;

    CimCurrent->Backend = Backend;
}

static cim_dx11_pipeline
CimDx11_CreatePipeline(cim_bit_field Features)
{
    cim_dx11_pipeline Pipeline = {};

    cim_u32          Enabled = 0;
    D3D_SHADER_MACRO Defines[CimFeature_Count + 1] = {};

    if (Features & CimFeature_AlbedoMap)
    {
        Defines[Enabled++] = (D3D_SHADER_MACRO){ "HAS_ALBEDO_MAP", "1" };
    }

    if (Features & CimFeature_MetallicMap)
    {
        Defines[Enabled++] = (D3D_SHADER_MACRO){ "HAS_METALLIC_MAP", "1" };
    }

    ID3DBlob *VSBlob = NULL;
    Pipeline.VtxShader = CimDx11_CreateVtxShader(Defines, &VSBlob); Cim_Assert(VSBlob);
    Pipeline.PxlShader = CimDx11_CreatePxlShader(Defines);
    Pipeline.Layout = CimDx11_CreateInputLayout(VSBlob, &Pipeline.Stride);

    CimDx11_Release(VSBlob);

    return Pipeline;
}

static cim_dx11_pipeline *
CimDx11_GetOrCreatePipeline(cim_bit_field Key, cim_dx11_pipeline_hashmap *Hashmap)
{
    if (!Hashmap->IsInitialized)
    {
        Hashmap->GroupCount = 32;

        cim_u32 BucketCount = Hashmap->GroupCount * CimBucketGroupSize;

        Hashmap->Buckets = (cim_dx11_entry *)malloc(BucketCount * sizeof(cim_dx11_entry));
        Hashmap->Metadata = (cim_u8 *)malloc(BucketCount * sizeof(cim_u8));

        if (!Hashmap->Buckets || !Hashmap->Metadata)
        {
            return NULL;
        }

        memset(Hashmap->Buckets, 0, BucketCount * sizeof(cim_dx11_entry));
        memset(Hashmap->Metadata, CimEmptyBucketTag, BucketCount * sizeof(cim_u8));

        Hashmap->IsInitialized = true;
    }

    cim_u32 ProbeCount = 0;
    cim_u32 HashedValue = CimHash_Block32(&Key, sizeof(Key));
    cim_u32 GroupIndex = HashedValue & (Hashmap->GroupCount - 1);

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
        cim_i32 MaskEmpty = _mm_movemask_epi8(_mm_cmpeq_epi8(MetaVector, EmptyVector));

        if (MaskEmpty)
        {
            cim_u32 Lane = CimHash_FindFirstBit32(MaskEmpty);
            cim_u32 Index = (GroupIndex * CimBucketGroupSize) + Lane;

            cim_dx11_entry *Entry = Hashmap->Buckets + Index;
            Entry->Key = Key;
            Entry->Value = CimDx11_CreatePipeline(Key);

            Meta[Lane] = Tag;

            return &Entry->Value;
        }

        ProbeCount++;
        GroupIndex = (GroupIndex + (ProbeCount * ProbeCount)) & (Hashmap->GroupCount - 1);
    }
}

static void
CimDx11_SetupRenderState(cim_i32 ClientWidth, cim_i32 ClientHeight, cim_dx11_backend *Backend)
{
    ID3D11Device *Device = Backend->Device;
    ID3D11DeviceContext *DeviceCtx = Backend->DeviceContext;

    if (!Backend->SharedFrameData)
    {
        D3D11_BUFFER_DESC Desc = {};
        Desc.ByteWidth = sizeof(cim_dx11_shared_data);
        Desc.Usage = D3D11_USAGE_DYNAMIC;
        Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        HRESULT Status = Device->CreateBuffer(&Desc, NULL, &Backend->SharedFrameData);
        Cim_AssertHR(Status);
    }

    D3D11_MAPPED_SUBRESOURCE MappedResource;
    if (DeviceCtx->Map(Backend->SharedFrameData, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource) == S_OK)
    {
        cim_dx11_shared_data *SharedData = (cim_dx11_shared_data *)MappedResource.pData;

        cim_f32 Left = 0;
        cim_f32 Right = ClientWidth;
        cim_f32 Top = 0;
        cim_f32 Bot = ClientHeight;

        cim_f32 SpaceMatrix[4][4] =
        {
            { 2.0f / (Right - Left)          , 0.0f                     , 0.0f, 0.0f },
            { 0.0f                           , 2.0f / (Top - Bot)       , 0.0f, 0.0f },
            { 0.0f                           , 0.0f                     , 0.5f, 0.0f },
            { (Right + Left) / (Left - Right), (Top + Bot) / (Bot - Top), 0.5f, 1.0f },
        };

        memcpy(&SharedData->SpaceMatrix, SpaceMatrix, sizeof(SpaceMatrix));
        DeviceCtx->Unmap(Backend->SharedFrameData, 0);
    }

    DeviceCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    DeviceCtx->IASetIndexBuffer(Backend->IdxBuffer, DXGI_FORMAT_R32_UINT, 0);
    DeviceCtx->VSSetConstantBuffers(0, 1, &Backend->SharedFrameData);

    D3D11_VIEWPORT Viewport;
    Viewport.TopLeftX = 0.0f;
    Viewport.TopLeftY = 0.0f;
    Viewport.Width = ClientWidth;
    Viewport.Height = ClientHeight;
    Viewport.MinDepth = 0.0f;
    Viewport.MaxDepth = 1.0f;
    DeviceCtx->RSSetViewports(1, &Viewport);
}

static void
CimDx11_RenderUI(cim_i32 ClientWidth, cim_i32 ClientHeight)
{
    Cim_Assert(CimCurrent);
    cim_dx11_backend *Backend = (cim_dx11_backend *)CimCurrent->Backend; Cim_Assert(Backend);

    if (ClientWidth == 0 || ClientHeight == 0)
    {
        return;
    }

    HRESULT              Status    = S_OK;
    ID3D11Device        *Device    = Backend->Device;        Cim_Assert(Device);
    ID3D11DeviceContext *DeviceCtx = Backend->DeviceContext; Cim_Assert(DeviceCtx);
    cim_cmd_buffer  *CmdBuffer = UIP_COMMANDS;

    if (!Backend->VtxBuffer || CmdBuffer->FrameVtx.At > Backend->VtxBufferSize)
    {
        CimDx11_Release(Backend->VtxBuffer);

        Backend->VtxBufferSize = CmdBuffer->FrameVtx.At + 1024;

        D3D11_BUFFER_DESC Desc = {};
        Desc.ByteWidth = Backend->VtxBufferSize;
        Desc.Usage = D3D11_USAGE_DYNAMIC;
        Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        Status = Device->CreateBuffer(&Desc, NULL, &Backend->VtxBuffer); Cim_AssertHR(Status);
    }

    if (!Backend->IdxBuffer || CmdBuffer->FrameIdx.At > Backend->IdxBufferSize)
    {
        CimDx11_Release(Backend->IdxBuffer);

        Backend->IdxBufferSize = CmdBuffer->FrameIdx.At + 1024;

        D3D11_BUFFER_DESC Desc = {};
        Desc.ByteWidth = Backend->IdxBufferSize;
        Desc.Usage = D3D11_USAGE_DYNAMIC;
        Desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        Status = Device->CreateBuffer(&Desc, NULL, &Backend->IdxBuffer); Cim_AssertHR(Status);
    }

    D3D11_MAPPED_SUBRESOURCE VtxResource = {};
    Status = DeviceCtx->Map(Backend->VtxBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &VtxResource); Cim_AssertHR(Status);
    if (FAILED(Status) || !VtxResource.pData)
    {
        return;
    }
    memcpy(VtxResource.pData, CmdBuffer->FrameVtx.Memory, CmdBuffer->FrameVtx.At);
    DeviceCtx->Unmap(Backend->VtxBuffer, 0);

    D3D11_MAPPED_SUBRESOURCE IdxResource = {};
    Status = DeviceCtx->Map(Backend->IdxBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &IdxResource); Cim_AssertHR(Status);
    if (FAILED(Status) || !IdxResource.pData)
    {
        return;
    }
    memcpy(IdxResource.pData, CmdBuffer->FrameIdx.Memory, CmdBuffer->FrameIdx.At);
    DeviceCtx->Unmap(Backend->IdxBuffer, 0);

    // ===============================================================================

    // Do we just inline this?
    CimDx11_SetupRenderState(ClientWidth, ClientHeight, Backend);

    // ===============================================================================
    for (cim_u32 CmdIdx = 0; CmdIdx < CmdBuffer->CommandCount; CmdIdx++)
    {
        cim_draw_command *Command = CmdBuffer->Commands + CmdIdx;
        cim_dx11_pipeline *Pipeline = CimDx11_GetOrCreatePipeline(Command->Features, &Backend->PipelineStore);

        Cim_Assert(Command && Pipeline);

        DeviceCtx->IASetInputLayout(Pipeline->Layout);
        DeviceCtx->IASetVertexBuffers(0, 1, &Backend->VtxBuffer, &Pipeline->Stride, &Pipeline->Offset);

        DeviceCtx->VSSetShader(Pipeline->VtxShader, NULL, 0);

        DeviceCtx->PSSetShader(Pipeline->PxlShader, NULL, 0);

        DeviceCtx->DrawIndexed(Command->IdxCount, Command->IdxOffset, Command->VtxOffset);
    }

    CimArena_Reset(&CmdBuffer->FrameVtx);
    CimArena_Reset(&CmdBuffer->FrameIdx);
    CmdBuffer->CommandCount = 0;
}

#endif