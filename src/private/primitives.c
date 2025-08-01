#include "cim_private.h"

#ifdef __cplusplus
extern "C" {
#endif

cim_point_node *
CimPrimitive_PushQuad(cim_point At, cim_u32 Width, cim_u32 Height, cim_primitive_rings *Rings)
{
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


#ifdef __cplusplus
}
#endif
