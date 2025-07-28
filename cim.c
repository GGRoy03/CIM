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

bool Cim_Window(const char *Id, cim_f32 *Color, cim_bit_field Flags)
{
    cim_context *Ctx = CimContext;
    Cim_Assert(Ctx && "Forgot to initialize context?");

    cim_component *Component = CimComponent_GetOrInsert();
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

    cim_point Head, Tail;
    cim_rect  HeaderRect, BodyRect;

    Head       = Window->Head->Value;
    Tail       = Window->Head->Prev->Value;
    HeaderRect = (cim_rect){Head.x, Head.y, Tail.x, Tail.y};
    CimCommand_PushQuad(HeaderRect, Color);

    Head     = Window->Body->Value;
    Tail     = Window->Body->Prev->Value;
    BodyRect = (cim_rect){Head.x, Head.y, Tail.x, Tail.y};
    CimCommand_PushQuad(BodyRect, Color);

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
