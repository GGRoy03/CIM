#include "cim.h"
#include "cim_private.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#include "third_party/stb_image.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ============================================================
// PUBLIC API TYPE DEFINITIONS FOR CIM. BY SECTION.
// -[SECTION:Widgets]
// -[SECTION:IO]
// ============================================================
// ============================================================

// [SECTION:Widgets] {

// NOTE: If we treat this as our state cache:

static cim_window CachedWindow;

bool Window(const char *Id, cim_bit_field Flags)
{
    // 1) First part is reading the inputs, doing some hit testing and updating
    // the retained state for that window.

    bool MouseDown = Cim_IsMouseDown(CimMouse_Left);

    cim_constraint Constraints[1] = {0};
    cim_u32        Count          = 0;

    if(MouseDown & (Flags & CimWindow_Draggable))
    {
        cim_context_draggable Context;
        Context.Holder = &CachedWindow.Holder;

        cim_constraint Draggable;
        Draggable.Apply   = CimConstraint_ApplyDraggable;
        Draggable.Context = &Context;

        CachedWindow.Constraints[Count++] = Draggable;
    }

    for(cim_u32 Idx = 0; Idx < Count; Idx++)
    {
        Constraints[Idx].Apply(Constraints[Idx].Context);
    }

    return true;
}

// } [SECTION:Widgets]

// [SECTION:IO] {

inline bool Cim_IsMouseDown(CimMouse_Button MouseButton)
{
    cim_context   *Ctx    = CimContext;
    cim_io_inputs *Inputs = &Ctx->Inputs;

    bool IsDown = Inputs->MouseButtons[MouseButton].EndedDown;
    return IsDown;
}

inline bool Cim_IsMouseReleased(CimMouse_Button MouseButton)
{
    cim_context   *Ctx    = CimContext;
    cim_io_inputs *Inputs = &Ctx->Inputs;

    cim_io_button_state *State = &Inputs->MouseButtons[MouseButton];
    bool IsReleased = (!State->EndedDown) && (State->HalfTransitionCount > 0);

    return IsReleased;
}

cim_f32 Cim_GetMouseDeltaX()
{
    cim_context   *Ctx    = CimContext;
    cim_io_inputs *Inputs = &Ctx->Inputs;

    cim_f32 DeltaX = Inputs->MouseDeltaX;
    return DeltaX;
}

cim_f32 Cim_GetMouseDeltaY()
{
    cim_context   *Ctx    = CimContext;
    cim_io_inputs *Inputs = &Ctx->Inputs;

    cim_f32 DeltaY = Inputs->MouseDeltaY;
    return DeltaY;
}

cim_vector2 Cim_GetMousePosition()
{
    cim_context   *Ctx    = CimContext;
    cim_io_inputs *Inputs = &Ctx->Inputs;

    cim_vector2 Position = (cim_vector2){(cim_f32)Inputs->MouseX, (cim_f32)Inputs->MouseY};
    return Position;
}

// } [SECTION:IO]

#ifdef __cplusplus
}
#endif
