#include "cim_dx11.h"

// ============================================================
// ============================================================
// DX11 IMPLEMENTATION FOR CIM. BY SECTION.
// -[SECTION] Shaders
// -[SECTION] Hashing
// -[SECTION] Commands
// ============================================================
// ============================================================

// -[SECTION:Shaders] {
// TODO: 
// 1) Make the shader compilation flags user based.

// WARN:
// 1) When creating a layout InstanceDataStepRate and InputSlot are always 0.

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
    HRESULT           Status  = S_OK;
    cim_context      *Ctx     = CimContext;
    cim_dx11_backend *Backend = Ctx->Backend;

    const char  *EntryPoint   = "VSMain";
    const char  *Profile      = "vs_5_0";
    const char  *ByteCode     = "";
    const SIZE_T ByteCodeSize = 0;

    // TODO: Make this user flag based.
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

static ID3D11InputLayout *
CimDx11_CreateInputLayout(cim_bit_field Features, ID3DBlob *VtxBlob, UINT *OutStride)
{
    ID3D11ShaderReflection *Reflection = NULL;
    D3DReflect(ID3D10Blob_GetBufferPointer(VtxBlob), ID3D10Blob_GetBufferSize(VtxBlob),
               &IID_ID3D11ShaderReflection, &Reflection);

    D3D11_SHADER_DESC ShaderDesc;
    Reflection->lpVtbl->GetDesc(Reflection, &ShaderDesc);

    D3D11_INPUT_ELEMENT_DESC Desc[32] = {0};
    for (cim_u32 InputIdx = 0; InputIdx < ShaderDesc.InputParameters; InputIdx++)
    {
        D3D11_SIGNATURE_PARAMETER_DESC Signature;
        Reflection->lpVtbl->GetInputParameterDesc(Reflection, InputIdx, &Signature);

        D3D11_INPUT_ELEMENT_DESC *InputDesc = Desc + InputIdx;
        InputDesc->SemanticName      = Signature.SemanticName;
        InputDesc->SemanticIndex     = Signature.SemanticIndex;
        InputDesc->AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
        InputDesc->InputSlotClass    = D3D11_INPUT_PER_VERTEX_DATA;

        if (Signature.Mask == 1)
        {
            if      (Signature.ComponentType == D3D_REGISTER_COMPONENT_UINT32)  InputDesc->Format = DXGI_FORMAT_R32_UINT;
            else if (Signature.ComponentType == D3D_REGISTER_COMPONENT_SINT32)  InputDesc->Format = DXGI_FORMAT_R32_SINT;
            else if (Signature.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) InputDesc->Format = DXGI_FORMAT_R32_FLOAT;
        }
        else if (Signature.Mask <= 3)
        {
            if      (Signature.ComponentType == D3D_REGISTER_COMPONENT_UINT32)  InputDesc->Format = DXGI_FORMAT_R32G32_UINT;
            else if (Signature.ComponentType == D3D_REGISTER_COMPONENT_SINT32)  InputDesc->Format = DXGI_FORMAT_R32G32_SINT;
            else if (Signature.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) InputDesc->Format = DXGI_FORMAT_R32G32_FLOAT;
        }
        else if (Signature.Mask <= 7)
        {
            if      (Signature.ComponentType == D3D_REGISTER_COMPONENT_UINT32)  InputDesc->Format = DXGI_FORMAT_R32G32B32_UINT;
            else if (Signature.ComponentType == D3D_REGISTER_COMPONENT_SINT32)  InputDesc->Format = DXGI_FORMAT_R32G32B32_SINT;
            else if (Signature.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) InputDesc->Format = DXGI_FORMAT_R32G32B32_FLOAT;
        }
        else if (Signature.Mask <= 15)
        {
            if      (Signature.ComponentType == D3D_REGISTER_COMPONENT_UINT32)  InputDesc->Format = DXGI_FORMAT_R32G32B32A32_UINT;
            else if (Signature.ComponentType == D3D_REGISTER_COMPONENT_SINT32)  InputDesc->Format = DXGI_FORMAT_R32G32B32A32_SINT;
            else if (Signature.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) InputDesc->Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        }
    }

    cim_context       *Ctx     = CimContext;
    cim_dx11_backend  *Backend = Ctx->Backend;
    HRESULT            Status  = S_OK;
    ID3D11InputLayout *Layout  = NULL;

    Status = ID3D11Device_CreateInputLayout(Backend->Device, Desc,
                                            ShaderDesc.InputParameters,
                                            ID3D10Blob_GetBufferPointer(VtxBlob),
                                            ID3D10Blob_GetBufferSize(VtxBlob),
                                            &Layout);
    Cim_AssertHR(Status);

    return Layout;
}

// } -[SECTION:Shaders]

// -[SECTION:Hashing] {
// WARN:
// 1) Contains a lot of duplicate code for now. That is because the full design is not
// yet set in stone.

static void CimDx11_InitializeGroupMap(cim_dx11_group_map *Map)
{
    Map->GroupCount = 8;

    cim_u32 BucketCount = Map->GroupCount * Dx11MapBucketGroupSize;
    size_t  BucketAllocationSize = BucketCount * sizeof(cim_dx11_group_entry);

    Map->Buckets = malloc(BucketAllocationSize);
    Map->MetaData = malloc(BucketCount * sizeof(cim_u8));

    if (!Map->Buckets || !Map->MetaData)
    {
        return;
    }

    memset(Map->Buckets, 0, BucketAllocationSize);
    memset(Map->MetaData, Dx11MapEmptyBucketTag, BucketCount * sizeof(cim_u8));
}

static void CimDx11_InsertGroup(cim_dx11_group_map *Map, const char *Key, cim_dx11_group Value)
{
    cim_u32 ProbeCount  = 0;
    cim_u32 HashedValue = Cim_HashString(Key);
    cim_u32 GroupIndex  = HashedValue & (Map->GroupCount - 1);

    while (true)
    {
        cim_u8 *Meta = Map->MetaData + (GroupIndex * Dx11MapBucketGroupSize);
        cim_u8  Tag = (HashedValue & 0x7F);

        Cim_Assert(Tag < Dx11MapEmptyBucketTag);

        __m128i MetaVector = _mm_loadu_si128((__m128i *)Meta);
        __m128i TagVector = _mm_set1_epi8(Tag);

        cim_i32 Mask = _mm_movemask_epi8(_mm_cmpeq_epi8(MetaVector, TagVector));

        while (Mask)
        {
            cim_u32 Lane  = Cim_FindFirstBit32(Mask);
            cim_u32 Index = (GroupIndex * Dx11MapBucketGroupSize) + Lane;

            cim_dx11_group_entry *Entry = Map->Buckets + Index;
            if (Entry->Key == Key)
            {
                return;
            }

            Mask &= Mask - 1;
        }

        __m128i EmptyVector = _mm_set1_epi8(Dx11MapEmptyBucketTag);
        cim_i32 MaskEmpty = _mm_movemask_epi8(_mm_cmpeq_epi8(MetaVector,
            EmptyVector));

        if (MaskEmpty)
        {
            cim_i32 Lane  = Cim_FindFirstBit32(MaskEmpty);
            cim_u32 Index = (GroupIndex * Dx11MapBucketGroupSize) + Lane;

            cim_dx11_group_entry *Entry = Map->Buckets + Index;
            strcpy_s(Entry->Key, sizeof(Entry->Key), Key);
            Entry->Value = Value;

            Meta[Lane] = Tag;

            return;
        }

        ProbeCount++;
        GroupIndex = (GroupIndex + (ProbeCount * ProbeCount)) & (Map->GroupCount - 1);
    }
}

static cim_dx11_group CimDx11_GetGroup(cim_dx11_group_map *Map, const char *Key)
{
    cim_u32 ProbeCount = 0;
    cim_u32 Hashed     = Cim_HashString(Key);
    cim_u32 GroupIndex = Hashed & (Map->GroupCount - 1);

    while (true)
    {
        cim_u8 *Meta = Map->MetaData + (GroupIndex * Dx11MapBucketGroupSize);
        cim_u8  Tag  = (Hashed & 0x7F);

        __m128i MetaVector = _mm_loadu_si128((__m128i *)Meta);
        __m128i TagVector  = _mm_set1_epi8(Tag);

        cim_i32 Mask = _mm_movemask_epi8(_mm_cmpeq_epi8(MetaVector, TagVector));

        while (Mask)
        {
            cim_u32 Lane  = Cim_FindFirstBit32(Mask);
            cim_u32 Index = (GroupIndex * Dx11MapBucketGroupSize) + Lane;

            cim_dx11_group_entry *Entry = Map->Buckets + Index;
            if (Entry->Key == Key)
            {
                return Entry->Value;
            }

            Mask &= Mask - 1;
        }

        __m128i EmptyVector = _mm_set1_epi8(Dx11MapEmptyBucketTag);
        cim_i32 MaskEmpty   = _mm_movemask_epi8(_mm_cmpeq_epi8(MetaVector, EmptyVector));

        if (MaskEmpty)
        {
            abort();
            return (cim_dx11_group){ 0 };
        }

        ProbeCount++;
        GroupIndex = (GroupIndex + (ProbeCount * ProbeCount)) & (Map->GroupCount - 1);
    }
}

static void CimDx11_DeleteGroup(cim_dx11_group_map *Map, const char *Key)
{
    cim_u32 ProbeCount = 0;
    cim_u32 Hashed     = Cim_HashString(Key);
    cim_u32 GroupIndex = Hashed & (Map->GroupCount - 1);

    while (true)
    {
        cim_u8 *Meta = Map->MetaData + (GroupIndex * Dx11MapBucketGroupSize);
        cim_u8  Tag  = (Hashed & 0x7F);

        __m128i MetaVector = _mm_loadu_si128((__m128i *)Meta);
        __m128i TagVector  = _mm_set1_epi8(Tag);

        cim_i32 Mask = _mm_movemask_epi8(_mm_cmpeq_epi8(MetaVector, TagVector));

        while (Mask)
        {
            cim_u32 Lane  = Cim_FindFirstBit32(Mask);
            cim_u32 Index = (GroupIndex * Dx11MapBucketGroupSize) + Lane;

            cim_dx11_group_entry *Entry = Map->Buckets + Index;
            if (Entry->Key == Key)
            {
                memset(Entry, 0, sizeof(cim_dx11_group_entry));
                Map->MetaData[Index] = Dx11MapEmptyBucketTag;
                return;
            }

            Mask &= Mask - 1;
        }

        __m128i EmptyVector = _mm_set1_epi8(Dx11MapEmptyBucketTag);
        cim_i32 MaskEmpty   = _mm_movemask_epi8(_mm_cmpeq_epi8(MetaVector, EmptyVector));

        if (MaskEmpty)
        {
            return;
        }

        ProbeCount++;
        GroupIndex = (GroupIndex + (ProbeCount * ProbeCount)) & (Map->GroupCount - 1);
    }
}

// } -[SECTION:Hashing]

// -[SECTION:Commands] {

// WARN:
// 1) The pointer casting in the playback function is possibly a failure point on
// different architectures. Or for the C standard. Need to look more into it.

static void CimDx11_CreateMaterial(cim_payload_create_material *Payload)
{
    cim_context      *Ctx     = CimContext;
    cim_dx11_backend *Backend = Ctx->Backend;
    cim_dx11_group    Group   = { 0 };

    D3D_SHADER_MACRO Defines[32] = { 0 };
    cim_u32          Enabled     = 0;
    cim_bit_field    Features    = Payload->Features;

    if (Features & CimMaterialFeature_Color)
    {
        Defines[Enabled++] = (D3D_SHADER_MACRO){"HAS_COLORS", "1"};
    }

    ID3DBlob *VSBlob = NULL;

    Group.VtxShader = CimDx11_CreateVtxShader(Defines, &VSBlob);
    Group.PxlShader = CimDx11_CreatePxlShader(Defines);
    Group.Layout    = CimDx11_CreateInputLayout(Features, VSBlob, &Group.Stride);

    CimDx11_InsertGroup(&Backend->GroupMap, Payload->UserID, Group);
}

static void CimDx11_DestroyMaterial(cim_payload_destroy_material *Payload)
{
    cim_context      *Ctx     = CimContext;
    cim_dx11_backend *Backend = Ctx->Backend;

    cim_dx11_group Group = CimDx11_GetGroup(&Backend->GroupMap, Payload->UserID);

    CimDx11_Release(Group.VtxBuffer);
    CimDx11_Release(Group.IdxBuffer);
    CimDx11_Release(Group.VtxShader);
    CimDx11_Release(Group.PxlShader);
    CimDx11_Release(Group.Layout);

    CimDx11_DeleteGroup(&Backend->GroupMap, Payload->UserID);
}

static void CimDx11_Playback(void *CommandBuffer, size_t CommandBufferSize)
{
    cim_context      *Ctx     = CimContext;
    cim_dx11_backend *Backend = Ctx->Backend;

    cim_u8 *CmdPtr = Ctx->PushBase;
    cim_u32 Offset = 0;

    while (Offset < Ctx->PushSize)
    {
        cim_render_command_header *Header = (cim_render_command_header*)(CmdPtr + Offset);
        Offset += sizeof(cim_render_command_header);

        switch (Header->Type)
        {

        case CimRenderCommand_CreateMaterial:
        {
            cim_payload_create_material *Payload = (cim_payload_create_material*)(CmdPtr + Offset);
            CimDx11_CreateMaterial(Payload);

        } break;

        case CimRenderCommand_DestroyMaterial:
        {
            cim_payload_destroy_material *Payload = (cim_payload_destroy_material*)(CmdPtr + Offset);
            CimDx11_DestroyMaterial(Payload);
        } break;

        }
    }
}

// } -[SECTION:Commands]

// NOTE: I really dislike this code. We have to get the device somehow.
void CimDx11_Initialize(ID3D11Device *UserDevice, ID3D11DeviceContext *UserContext)
{
    cim_context      *Ctx     = CimContext;
    cim_dx11_backend *Backend = malloc(sizeof(cim_dx11_backend));

    if (!Backend)
    {
        return;
    }

    Backend->Device        = UserDevice;
    Backend->DeviceContext = UserContext;
    CimDx11_InitializeGroupMap(&Backend->GroupMap);
    Ctx->Backend = Backend;

    Cim_CreateMaterial("Default", CimMaterialFeature_Color);
}