#include "cim.h"
#include "cim_private.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ============================================================
// PRIVATE IMPLEMENTATION FOR CIM. BY SECTION.
// -[SECTION:Hashing]
// -[SECTION:Constraints]
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

// [SECTION:Constraints] {

void CimConstraint_ApplyDraggable(void *Context)
{
    cim_context_draggable *Ctx = (cim_context_draggable*)Context;

    cim_holder *Holder = Ctx->Holder;
    cim_vector2 Mouse  = Cim_GetMousePosition();
    cim_f32     DeltaX = Cim_GetMouseDeltaX();
    cim_f32     DeltaY = Cim_GetMouseDeltaY();
}

// } [SECTION:Constraints]

cim_context *CimContext;

#ifdef __cplusplus
}
#endif
