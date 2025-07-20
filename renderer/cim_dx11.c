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
    Status = ID3D11Device_CreatePixelShader(Backend->Device, ID3D10Blob_GetBufferPointer(Blob),
                                            ID3D10Blob_GetBufferSize(Blob), NULL, &PixelShader);
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

        // TODO: By reading ComponentType and Mask from signature we should be able to figure out the format.
        DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;

        // WARN: InstanceDataStepRate and InputSlot are always 0 for now.

        D3D11_INPUT_ELEMENT_DESC *InputDesc = Desc + InputIdx;
        InputDesc->SemanticName      = Signature.SemanticName;
        InputDesc->SemanticIndex     = Signature.SemanticIndex;
        InputDesc->Format            = Format;
        InputDesc->AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
        InputDesc->InputSlotClass    = D3D11_INPUT_PER_VERTEX_DATA;
    }

    cim_context       *Ctx     = CimContext;
    cim_dx11_backend  *Backend = Ctx->Backend;
    HRESULT            Status  = S_OK;
    ID3D11InputLayout *Layout  = NULL;

    Status = ID3D11Device_CreateInputLayout(Backend->Device, Desc, ShaderDesc.InputParameters,
                                            ID3D10Blob_GetBufferPointer(VtxBlob), ID3D10Blob_GetBufferSize(VtxBlob),
                                            &Layout);
    Cim_AssertHR(Status);

    return Layout;
}

// } -[SECTION:Shaders]

// -[SECTION:Hashing] {
// 1) Contains a lot of duplicate code for now. That is because the full design is not yet set in stone.

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

// } -[SECTION:Hashing]

// -[SECTION:Commands] {

// WARN:
// 1) The pointer casting in the playback function is possibly a failure point on different architectures.
// / for the C standard. Need to look more into it.

void CimDx11_CreateMaterial(cim_payload_create_material *Payload)
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

void CimDx11_DestroyMaterial(cim_payload_destroy_material *Payload)
{
    cim_context      *Ctx     = CimContext;
    cim_dx11_backend *Backend = Ctx->Backend;

    cim_dx11_group Group = CimDx11_GetGroup(&Backend->GroupMap, Payload->UserID);
    (void)Group;
    
    // TODO: Release the dx11 objects and remove the entry from the hashmap.
}

void CimDx11_Playback(void *CommandBuffer, size_t CommandBufferSize)
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

        } break;

        }
    }
}

// } -[SECTION:Commands]

void CimDx11_Initialize(ID3D11Device *UserDevice, ID3D11DeviceContext *UserContext)
{
    cim_context      *Ctx = CimContext;
    cim_dx11_backend *Backend = malloc(sizeof(cim_dx11_backend));

    if (!Backend)
    {
        return;
    }

    Backend->Device        = UserDevice;
    Backend->DeviceContext = UserContext;
    CimDx11_InitializeGroupMap(&Backend->GroupMap);
    Ctx->Backend = Backend;

    // NOTE: Here's the tricky part, we have to create the material.
    // I think we just use the same or almost the same scheme we used
    // for the engine.

}
