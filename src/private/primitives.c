#include "cim_private.h"

#ifdef __cplusplus
extern "C" {
#endif

cim_point_node *
AllocateQuad(cim_point At, cim_f32 Width, cim_f32 Height)
{
    cim_context         *Ctx   = CimContext; Cim_Assert(Ctx);
    cim_primitive_rings *Rings = &Ctx->PrimitiveRings;

    cim_point_node *TopLeft     = Rings->PointNodes + Rings->PointCount++;
    cim_point_node *BottomRight = Rings->PointNodes + Rings->PointCount++;

    TopLeft->Prev  = BottomRight;
    TopLeft->Next  = BottomRight;
    TopLeft->Value = At;

    BottomRight->Prev  = TopLeft;
    BottomRight->Next  = TopLeft;
    BottomRight->Value = (cim_point){At.x + Width, At.y  + Height};

    return TopLeft;
}

void 
ReplaceQuad(cim_point_node *ToReplace, cim_point TopLeft, cim_f32 Width, cim_f32 Height)
{
    cim_point_node *TopLeftPt  = ToReplace;
    cim_point_node *BotRightPt = ToReplace->Prev;

    TopLeftPt->Value  = TopLeft;
    BotRightPt->Value = (cim_point){ TopLeft.x + Width, TopLeft.y + Height };
}

#ifdef __cplusplus
}
#endif
