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

    cim_primitive_rings *Rings = &Ctx->PrimitiveRings;
    cim_state_node      *State = CimMap_GetStateValue(Id, &Rings->StateMap);

    if(!State)
    {
        // Create default state
        cim_ui_state State = { 0 };
        State.Type              = CimUIState_Window;
        State.For.Window.Closed = true; 

        // Add the state to the ring and the id-state map.
        cim_state_node *Node = CimRing_AddStateNode(State, Rings);
        CimMap_AddStateEntry(Id, Node, &Rings->StateMap);
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
