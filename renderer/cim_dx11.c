#include "cim_dx11.h"
#include "cim_private.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ============================================================
// DX11 IMPLEMENTATION FOR CIM. BY SECTION.
// -[SECTION] Shaders
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

// [SECTION:Commands] {
// WARN:
// 1) Should probably crash on malloc failure?
// 2) We are using a lot of memory.

// TODO:
// Implement the buffers: Memory copy, Binding, Updates, Offset draws ]
// Implement the shaders: Hashmap, Creation, Binding ]

void CimDx11_RenderUI()
{
    cim_context      *Ctx     = CimContext;                       Cim_Assert(Ctx);
    cim_dx11_backend *Backend = (cim_dx11_backend*)&Ctx->Backend; Cim_Assert(Backend);

    HRESULT              Status    = S_OK;
    ID3D11Device        *Device    = Backend->Device;        Cim_Assert(Device);
    ID3D11DeviceContext *DeviceCtx = Backend->DeviceContext; Cim_Assert(DeviceCtx);

    cim_command_buffer *CmdBuffer = &Ctx->CmdBuffer; Cim_Assert(CmdBuffer);

    // 1) Check the buffers validity (Are they big enough/initialized?)

    if (!Backend->VtxBuffer || CmdBuffer->FrameVtx.Size > Backend->VtxBufferSize)
    {
        if (Backend->VtxBuffer)
        {
            CimDx11_Release(Backend->VtxBuffer);
        }

        Backend->VtxBufferSize = CmdBuffer->FrameVtx.Size + 1024; // NOTE: This number is TBD.

        D3D11_BUFFER_DESC Desc = { 0 };
        Desc.ByteWidth      = Backend->VtxBufferSize;
        Desc.Usage          = D3D11_USAGE_DYNAMIC;
        Desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
        Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        Status = ID3D11Device_CreateBuffer(Device, &Desc, NULL, &Backend->VtxBuffer); Cim_AssertHR(Status);
    }

    if (!Backend->IdxBuffer || CmdBuffer->FrameIdx.Size > Backend->VtxBufferSize)
    {
        if (Backend->VtxBuffer)
        {
            CimDx11_Release(Backend->VtxBuffer);
        }

        Backend->VtxBufferSize = CmdBuffer->FrameVtx.Size + 1024; // NOTE: This number is TBD.

        D3D11_BUFFER_DESC Desc = { 0 };
        Desc.ByteWidth      = Backend->VtxBufferSize;
        Desc.Usage          = D3D11_USAGE_DYNAMIC;
        Desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
        Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        Status = ID3D11Device_CreateBuffer(Device, &Desc, NULL, &Backend->VtxBuffer); Cim_AssertHR(Status);
    }


    CimArena_Byte_Reset(&CmdBuffer->FrameVtx);
    CimArena_Idx_Reset (&CmdBuffer->FrameIdx);
}

// } [SECTION:Commands]

#ifdef __cplusplus
}
#endif
