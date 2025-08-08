#include "cim_private.h"

#ifdef __cplusplus
extern "C" {
#endif

// NOTE: Simply because of the way the data is accessed, we might want to
// remove the concept of a global. There's a lot of annoying ways we can
// repeat the same code across functions.

bool
IsInsideRect(cim_rect Rect)
{
    cim_vector2 MousePos = GetMousePosition();

    bool MouseIsInside = (MousePos.x > Rect.Min.x) && (MousePos.x < Rect.Max.x) &&
                         (MousePos.y > Rect.Min.y) && (MousePos.y < Rect.Max.y);

    return MouseIsInside;
}

#ifdef __cplusplus
}
#endif
