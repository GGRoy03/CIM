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

    Ctx->CmdBuffer.ClippingRectChanged = true;

    cim_component *Component = CimComponent_GetOrInsert(Id, &Ctx->ComponentStore);
    cim_window    *Window    = &Component->For.Window;

    if(Component->Type == CimComponent_Unknown)
    {
        Component->Type = CimComponent_Window;

        // Set the default state
        Window->IsClosed = false;

        // Set the retained data
        cim_point hp0 = (cim_point){500.0f, 500.0f};
        cim_point hp1 = (cim_point){500.0f, 800.0f};
        cim_point hp2 = (cim_point){800.0f, 500.0f};
        cim_point hp3 = (cim_point){800.0f, 600.0f};
        Window->Head = CimPoint_PushQuad(hp0, hp1, hp2, hp3);

        cim_point bp0 = (cim_point){500.0f, 600.0f};
        cim_point bp1 = (cim_point){500.0f, 950.0f};
        cim_point bp2 = (cim_point){800.0f, 600.0f};
        cim_point bp3 = (cim_point){800.0f, 950.0f};
        Window->Body = CimPoint_PushQuad(bp0, bp1, bp2, bp3);
    }

    if(Flags & CimWindow_Draggable)
    {
        cim_draggable Draggable = { {Window->Head, Window->Body }, 2};
        Drag[DragCount++] = Draggable;
    }

    CimCommand_PushQuadEntry(Window->Head, Color);

    if(!Window->IsClosed)
    {
        cim_vector4 BodyColor = { 0.3f, 0.3f, 0.3f, 1.0f };
        CimCommand_PushQuadEntry(Window->Body, BodyColor);
    }

    return true;
}

// } [SECTION:Widgets]

// [SECTION:IO] {

bool 
CimInput_IsMouseDown(CimMouse_Button MouseButton)
{
    cim_context   *Ctx    = CimContext;   Cim_Assert(Ctx);
    cim_io_inputs *Inputs = &Ctx->Inputs;

    bool IsDown = Inputs->MouseButtons[MouseButton].EndedDown;
    return IsDown;
}

bool 
CimInput_IsMouseReleased(CimMouse_Button MouseButton)
{
    cim_context   *Ctx    = CimContext;   Cim_Assert(Ctx);
    cim_io_inputs *Inputs = &Ctx->Inputs;

    cim_io_button_state *State = &Inputs->MouseButtons[MouseButton];
    bool IsReleased = (!State->EndedDown) && (State->HalfTransitionCount > 0);

    return IsReleased;
}

bool
CimInput_IsMouseClicked(CimMouse_Button MouseButton)
{
    cim_context   *Ctx    = CimContext;   Cim_Assert(Ctx);
    cim_io_inputs *Inputs = &Ctx->Inputs;

    cim_io_button_state *State = &Inputs->MouseButtons[MouseButton];
    bool IsClicked = (State->EndedDown) && (State->HalfTransitionCount > 0);

    return IsClicked;
}

cim_i32 
CimInput_GetMouseDeltaX(void)
{
    cim_context   *Ctx    = CimContext;
    cim_io_inputs *Inputs = &Ctx->Inputs;

    cim_f32 DeltaX = Inputs->MouseDeltaX;
    return DeltaX;
}

cim_i32 
CimInput_GetMouseDeltaY(void)
{
    cim_context   *Ctx    = CimContext;
    cim_io_inputs *Inputs = &Ctx->Inputs;

    cim_i32 DeltaY = Inputs->MouseDeltaY;
    return DeltaY;
}

cim_vector2 
CimInput_GetMousePosition(void)
{
    cim_context   *Ctx    = CimContext;
    cim_io_inputs *Inputs = &Ctx->Inputs;

    cim_vector2 Position = (cim_vector2){(cim_f32)Inputs->MouseX, (cim_f32)Inputs->MouseY};
    return Position;
}

// } [SECTION:IO]

// NOTE: Maybe we make the RenderUI function call end frame?
void
Cim_EndFrame()
{
    cim_context *Ctx = CimContext; Cim_Assert(Ctx);

    // Reset the inputs (Still unsure if the loops are correct)
    cim_io_inputs *Inputs = &Ctx->Inputs;
    Inputs->ScrollDelta = 0;
    Inputs->MouseDeltaX = 0;
    Inputs->MouseDeltaY = 0;

    for (cim_u32 Idx = 0; Idx < CIM_KEYBOARD_KEY_COUNT; Idx++)
    {
        Inputs->Buttons[Idx].HalfTransitionCount = 0;
    }

    for (cim_u32 Idx = 0; Idx < CimMouse_ButtonCount; Idx++)
    {
        Inputs->MouseButtons[Idx].HalfTransitionCount = 0;
    }
}

#ifdef __cplusplus
}
#endif
