#include "cim_private.h"

#ifdef __cplusplus
extern "C" {
#endif

bool
CimGeometry_HitTestRect(cim_rect    Rect,
                        cim_vector2 MousePos)
{
    bool MouseIsInside = (MousePos.x > Rect.Min.x) && (MousePos.x < Rect.Max.x) &&
                         (MousePos.y > Rect.Min.y) && (MousePos.y < Rect.Max.y);

    return MouseIsInside;
}

#ifdef __cplusplus
}
#endif
