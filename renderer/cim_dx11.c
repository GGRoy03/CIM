#include "cim_dx11.h"

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

void CimDx11_WalkCommandList(cim_command_stream *Commands)
{
    Cim_Assert(Commands);

    cim_context      *Ctx     = CimContext;
    cim_dx11_backend *Backend = (cim_dx11_backend*)&Ctx->Backend;

    HRESULT              Status        = S_OK;
    ID3D11Device        *Device        = Backend->Device;
    ID3D11DeviceContext *DeviceContext = Backend->DeviceContext;

    for(cim_u32 RingIdx = 0; RingIdx < Commands->RingCount; RingIdx++)
    {
        cim_command_ring *Ring     = Commands->Rings + RingIdx;
        cim_command      *Command  = Ring->CmdStart;
        cim_command      *RingHead = Command;

        cim_dx11_batch_resource *Resource = Backend->BatchResources + RingIdx;

        if(!Resource->VtxBuffer || 0 < Resource->VtxBufferSize)
        {
            Resource->VtxBufferSize = Ring->VtxSize + 0;

            if(Resource->VtxBuffer)
            {
                CimDx11_Release(Resource->VtxBuffer);
                Resource->VtxBuffer = NULL;
            }

            if(Resource->FrameVtxData)
            {
                free(Resource->FrameVtxData);
            }

            D3D11_BUFFER_DESC Desc = { 0 };
            Desc.ByteWidth      = Resource->VtxBufferSize;
            Desc.Usage          = D3D11_USAGE_DYNAMIC;
            Desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
            Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

            Status = ID3D11Device_CreateBuffer(Device, &Desc, NULL, &Resource->VtxBuffer);
            Cim_AssertHR(Status);

            Resource->FrameVtxData = malloc(Resource->VtxBufferSize);
            Cim_Assert(Resource->FrameVtxData);
        }

        if(!Resource->IdxBuffer || 0 < Resource->IdxBufferSize)
        {
            Resource->IdxBufferSize = Ring->IdxSize + 0;

            if (Resource->IdxBuffer)
            {
                CimDx11_Release(Resource->IdxBuffer);
                Resource->IdxBuffer = NULL;
            }

            if(Resource->FrameIdxData)
            {
                free(Resource->FrameIdxData);
            }

            D3D11_BUFFER_DESC Desc = { 0 };
            Desc.ByteWidth      = Resource->IdxBufferSize;
            Desc.Usage          = D3D11_USAGE_DYNAMIC;
            Desc.BindFlags      = D3D11_BIND_INDEX_BUFFER;
            Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

            Status = ID3D11Device_CreateBuffer(Device, &Desc, NULL, &Resource->IdxBuffer);
            Cim_AssertHR(Status);

            Resource->FrameIdxData = malloc(Resource->IdxBufferSize);
            Cim_Assert(Resource->FrameIdxData);
        }

        cim_u32 VtxOffset = 0;
        cim_u32 IdxCount  = 0;
        cim_u32 IdxBase   = 0;

        do
        {
            switch(Command->Type)
            {

            case CimTopo_Quad:
            {
                cim_point_node *Point = Command->For.Quad.Start;
                for(cim_u32 Corner = 0; Corner < 4; Corner++)
                {
                    void *WritePointer = (cim_u8*)Resource->FrameVtxData + VtxOffset;
                    memcpy(WritePointer, &Point->Value, sizeof(Point->Value));

                    VtxOffset += sizeof(Point->Value);
                    Point      = Point->Next;
                }

                Resource->FrameIdxData[IdxCount++] = IdxBase + 0;
                Resource->FrameIdxData[IdxCount++] = IdxBase + 1;
                Resource->FrameIdxData[IdxCount++] = IdxBase + 2;
                Resource->FrameIdxData[IdxCount++] = IdxBase + 2;
                Resource->FrameIdxData[IdxCount++] = IdxBase + 1;
                Resource->FrameIdxData[IdxCount++] = IdxBase + 3;

                IdxBase += 4; 
            } break;

            }

            Command = Command->Next;

        } while (Command != RingHead);

        D3D11_MAPPED_SUBRESOURCE VtxResource = {0};
        Status = ID3D11DeviceContext_Map(DeviceContext, Resource->VtxBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &VtxResource);
        Cim_AssertHR(Status);
        memcpy(VtxResource.pData, Resource->FrameVtxData, VtxOffset);
        ID3D11DeviceContext_Unmap(DeviceContext, Resource->VtxBuffer, 0);

        D3D11_MAPPED_SUBRESOURCE IdxResource = {0};
        Status = ID3D11DeviceContext_Map(DeviceContext, Resource->IdxBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &IdxResource);
        Cim_AssertHR(Status);
        memcpy(IdxResource.pData, Resource->FrameIdxData, IdxCount * sizeof(cim_u32));
        ID3D11DeviceContext_Unmap(DeviceContext, Resource->IdxBuffer, 0);

        ID3D11DeviceContext_DrawIndexed(DeviceContext, IdxCount, 0, 0);
    }
}

// } [SECTION:Commands]
