#include "cim.h"
#include "cim_private.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ============================================================
// PRIVATE IMPLEMENTATION FOR CIM. BY SECTION.
// -[SECTION] Hashing
// -[SECTION] Commands
// ============================================================
// ============================================================

// -[SECTION] Hashing {

cim_u32 Cim_FindFirstBit32(cim_u32 Mask)
{
    unsigned long Index = 0;
    _BitScanForward(&Index, Mask);
    return (cim_u32)Index;
}

cim_u32 Cim_HashString(const char* String)
{
    if (!String)
    {
        Cim_Assert(!"Cannot hash empty string.");
        return 0;
    }

    cim_u32 Result = FNV32Basis;
    cim_u8  Character;

    while ((Character = *String++))
    {
        Result = FNV32Prime * (Result ^ Character);
    }

    return Result;
}

// } -[SECTION:Hashing]

// -[SECTION:Commands] {

void CimInt_PushRenderCommand(cim_render_command_header *Header, void *Payload)
{
    cim_context *Ctx = CimContext;

    size_t SizeNeeded = sizeof(cim_render_command_header) + Header->Size;
    if (Ctx->PushSize + SizeNeeded <= Ctx->PushCapacity)
    {
        memcpy(Ctx->PushBase + Ctx->PushSize, Header, sizeof(cim_render_command_header));
        Ctx->PushSize += sizeof(cim_render_command_header);

        memcpy(Ctx->PushBase + Ctx->PushSize, Payload, Header->Size);
        Ctx->PushSize += Header->Size;
    }
    else
    {
        // NOTE: Do we resize, maybe user chooses? Doesn't matter for now.
        // Just abort.
        abort();
    }
}

// } -[SECTION:Commands]

cim_context *CimContext;

#ifdef __cplusplus
}
#endif
