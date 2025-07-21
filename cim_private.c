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

void CimConstraint_Register(CimConstraint_Type ConstType, void *Context)
{
    cim_context            *Ctx     = CimContext;
    cim_constraint_manager *Manager = &Ctx->ConstraintManager;

    switch(ConstType)
    {

    case CimConstraint_Drag:
    {
        cim_context_drag *WritePointer = &Manager->DragCtxs[Manager->RegDragCtxs++];
        memcpy(WritePointer, Context, sizeof(cim_context_drag));
    } break;

    default:
    {

    } break;

    }
}

void CimConstraint_SolveAll()
{
    cim_context            *Ctx     = CimContext;
    cim_constraint_manager *Manager = &Ctx->ConstraintManager;

    bool    MouseDown   = Cim_IsMouseDown(CimMouse_Left);
    cim_f32 MouseDeltaX = Cim_GetMouseDeltaX();
    cim_f32 MouseDeltaY = Cim_GetMouseDeltaY();

    // WARN: There's so much more to this, but a basic implementation
    // should look like this.
    if(MouseDown)
    {
        cim_f32 DragSpeed = 0.5f;
        cim_f32 DragX     = MouseDeltaX * DragSpeed;
        cim_f32 DragY     = MouseDeltaY * DragSpeed;

        for(cim_u32 CIdx = 0; CIdx < Manager->RegDragCtxs; CIdx++)
        {
            cim_context_drag *ConstCtx = Manager->DragCtxs + CIdx;
            cim_holder       *Holder   = ConstCtx->Holder;

            Cim_Assert(Holder->Type == CimHolder_Moving);

            for(cim_u32 PointIdx = 0; PointIdx < Holder->PointCount; PointIdx++)
            {
                cim_point *PrimitivePoint = Holder->Points + PointIdx;
                PrimitivePoint->x += DragX;
                PrimitivePoint->y += DragY;
            }
        }
    }
    Manager->RegDragCtxs = 0;
}

// } [SECTION:Constraints]

cim_context *CimContext;

#ifdef __cplusplus
}
#endif
