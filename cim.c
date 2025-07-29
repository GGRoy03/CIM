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

bool Cim_Window(const char *Id, cim_vector4 Color, cim_bit_field Flags)
{
    cim_context *Ctx = CimContext; Cim_Assert(Ctx && "Forgot to initialize context?");

    Ctx->CmdBuffer.ClippingRectChanged = true; // We force a new batch on a new window.

    cim_component *Component = CimComponent_GetOrInsert(Id, &Ctx->ComponentStore);
    cim_window    *Window    = &Component->For.Window;

    if(Component->Type == CimComponent_Unknown)
    {
        Component->Type = CimComponent_Window;

        // Set the default state
        Window->IsClosed = false;

        // Set the retained data
        cim_point hp0 = (cim_point){-0.50f, 0.33f};
        cim_point hp1 = (cim_point){-0.50f, 0.00f};
        cim_point hp2 = (cim_point){-0.25f, 0.33f};
        cim_point hp3 = (cim_point){-0.25f, 0.00f};
        Window->Head = CimPoint_PushQuad(hp0, hp1, hp2, hp3);

        cim_point bp0 = (cim_point){-0.50f,  0.0f};
        cim_point bp1 = (cim_point){-0.50f, -1.0f};
        cim_point bp2 = (cim_point){-0.25f,  0.0f};
        cim_point bp3 = (cim_point){-0.25f, -1.0f};
        Window->Body = CimPoint_PushQuad(bp0, bp1, bp2, bp3);
    }

    if(Flags & CimWindow_Draggable)
    {
        // NOTE: Here is where we apply the drag constraint. The idea is to register
        // the geometrical context. Oh fuck. We have a huge problem in the data-flow
        // because, we register the rect instantly. My model is kind of built to handle
        // the constraint instantly. If I defer the constraint solving, such that we
        // batch them, I have to go back to a command-type based renderer.
        // We don't really want the renderer to have a pointer to a point. We want
        // nice data locality. But the problem with deferring is that the constraints
        // have to modify the behavior of the renderer. Unless we make things like
        // "show" a constraint and the constraints are the one emitting the commands.
        // If we do that then we can also, batch behavorial constraints....
    }

    CimCommand_PushQuadEntry(Window->Head, Color);
    CimCommand_PushQuadEntry(Window->Body, Color);

    return true;
}

// } [SECTION:Widgets]

// [SECTION:IO] {

bool Cim_IsMouseDown(CimMouse_Button MouseButton)
{
    cim_context   *Ctx    = CimContext;
    cim_io_inputs *Inputs = &Ctx->Inputs;

    bool IsDown = Inputs->MouseButtons[MouseButton].EndedDown;
    return IsDown;
}

bool Cim_IsMouseReleased(CimMouse_Button MouseButton)
{
    cim_context   *Ctx    = CimContext;
    cim_io_inputs *Inputs = &Ctx->Inputs;

    cim_io_button_state *State = &Inputs->MouseButtons[MouseButton];
    bool IsReleased = (!State->EndedDown) && (State->HalfTransitionCount > 0);

    return IsReleased;
}

cim_f32 Cim_GetMouseDeltaX(void)
{
    cim_context   *Ctx    = CimContext;
    cim_io_inputs *Inputs = &Ctx->Inputs;

    cim_f32 DeltaX = Inputs->MouseDeltaX;
    return DeltaX;
}

cim_f32 Cim_GetMouseDeltaY(void)
{
    cim_context   *Ctx    = CimContext;
    cim_io_inputs *Inputs = &Ctx->Inputs;

    cim_f32 DeltaY = Inputs->MouseDeltaY;
    return DeltaY;
}

cim_vector2 Cim_GetMousePosition(void)
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
