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

bool Window(const char *Id, cim_bit_field Flags)
{
    // WARN: Do not do this here!
    if (!CimContext)
    {
        CimContext = malloc(sizeof(cim_context));
        if (CimContext)
        {
            memset(CimContext, 0, sizeof(cim_context));
        }
        else
        {
            Cim_Assert(!"DOA");
        }
    }

    cim_context *Ctx = CimContext;
    Cim_Assert(Ctx && "Forgot to initialize context?");

    cim_primitive_rings *Rings     = &Ctx->PrimitiveRings;
    cim_state_node      *StateNode = CimMap_GetStateValue(Id, &Rings->StateMap);

    if(!StateNode)
    {
        cim_ui_state      State  = { 0 };
        cim_window_state *Window = &State.For.Window;

        // Set default state
        State.Type     = CimUIState_Window;
        Window->Closed = false; 

        // Set the header state
        cim_point hp0 = (cim_point){200.0f, 200.0f};
        cim_point hp1 = (cim_point){200.0f, 300.0f};
        cim_point hp2 = (cim_point){300.0f, 200.0f};
        cim_point hp3 = (cim_point){300.0f, 300.0f};
        Window->Head = CimRing_PushQuad(hp0, hp1, hp2, hp3, Rings);

        // set the body state
        cim_point bp0 = (cim_point){200.0f, 300.0f};
        cim_point bp1 = (cim_point){200.0f, 600.0f};
        cim_point bp2 = (cim_point){300.0f, 200.0f};
        cim_point bp3 = (cim_point){300.0f, 600.0f};
        Window->Body = CimRing_PushQuad(bp0, bp1, bp2, bp3, Rings);

        // Add the state to the ring and the id-state map.
        StateNode = CimRing_AddStateNode(State, Rings);
        CimMap_AddStateEntry(Id, StateNode, &Rings->StateMap);

        Window->HeadCmd = CimCommand_PushQuad  (Window->Head);
        Window->BodyCmd = CimCommand_AppendQuad(Window->Body, Window->Head);
    }

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
