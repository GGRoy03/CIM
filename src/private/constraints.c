#include "cim_private.h"

#include <float.h>
#include <math.h>

cim_draggable Drag[4];
cim_u32       DragCount;

#ifdef __cplusplus
extern "C" {
#endif

// NOTE: Isn't this just way easier if the context itself contains the rect?
// Because the one providing the context, knows more about the context than this
// function and can easily take shortcuts to provide the correct rect.

static void
CimConstraint_SolveDraggable(cim_draggable *Registered,
                             cim_u32        RegisteredCount,
                             cim_i32        MouseDeltaX,
                             cim_i32        MouseDeltaY)
{
    for(cim_u32 RegIdx = 0; RegIdx < RegisteredCount; RegIdx++)
    {
        cim_draggable *Reg = Registered + RegIdx;

        // NOTE: This can be simplified.
        cim_rect Rect = (cim_rect){{FLT_MAX, FLT_MAX}, {FLT_MIN, FLT_MIN}};
        for(cim_u32 RingIdx = 0; RingIdx < Reg->Count; RingIdx++)
        {
            cim_point_node *Point = Reg->PointRings[RingIdx];
            do
            {
                Rect.Min.x = fminf(Rect.Min.x, Point->Value.x);
                Rect.Min.y = fminf(Rect.Min.y, Point->Value.y);
                Rect.Max.x = fmaxf(Rect.Max.x, Point->Value.x);
                Rect.Max.y = fmaxf(Rect.Max.y, Point->Value.y);

                Point = Point->Next;
            } while(Point != Reg->PointRings[RingIdx]);
        }

        if (IsInsideRect(Rect))
        {
            for (cim_u32 RingIdx = 0; RingIdx < Reg->Count; RingIdx++)
            {
                cim_point_node *Point = Reg->PointRings[RingIdx];
                do
                {
                    Point->Value.x += MouseDeltaX;
                    Point->Value.y += MouseDeltaY;

                    Point = Point->Next;
                } while (Point != Reg->PointRings[RingIdx]);
            }
        }

    }
}

void
CimConstraint_Solve(cim_inputs *Inputs)
{
    bool MouseDown = IsMouseDown(CimMouse_Left);

    cim_i32     MDeltaX = CimInput_GetMouseDeltaX(Inputs);
    cim_i32     MDeltaY = CimInput_GetMouseDeltaY(Inputs);
    cim_vector2 MPos    = GetMousePosition();

    if(MouseDown)
    {
        CimConstraint_SolveDraggable(Drag, DragCount, MDeltaX, MDeltaY);
    }

    DragCount = 0;
}


#ifdef __cplusplus
}
#endif
