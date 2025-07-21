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
    CachedWindow.Opened = true;

    // 1) First part is reading the inputs, doing some hit testing and updating
    // the retained state for that window.

    cim_point Mouse     = (cim_point){100, 100};
    bool      MouseDown = false;

    if(MouseDown) // WARN: Also needs to hit-test against the close button.
    {
        CachedWindow.Opened = false;
    }

    if(!CachedWindow.Opened)
    {
        return false;
    }

    // 2) Second part is solving the constraints. So let's be really naive for
    // now and say that we want to always re-build our constraints array and we also
    // want to always solve the constraints. We probably always want to force some
    // constraints.

    if(MouseDown) // WARN: We also want to hit-test on the window itself
    {
        CachedWindow.Constraints[0] = NULL;
    }

    // Here we have to run them somehow.

    // 3) Final step is emitting the topos. We'd say like if show_header, emit quad
    // for header. If window_opened emit quad for window. If wants_border emit
    // border topo

    return true;
}

// } [SECTION:Widgets]

// [SECTION:IO] {

inline bool IsMouseDown(CimMouse_Button MouseButton)
{
    cim_context   *Ctx    = CimContext;
    cim_io_inputs *Inputs = &Ctx->Inputs;

    bool IsDown = Inputs->MouseButtons[MouseButton].EndedDown;
    return IsDown;
}

inline bool IsMouseReleased(CimMouse_Button MouseButton)
{
    cim_context   *Ctx    = CimContext;
    cim_io_inputs *Inputs = &Ctx->Inputs;

    cim_io_button_state *State = &Inputs->MouseButtons[MouseButton];
    bool IsReleased = (!State->EndedDown) && (State->HalfTransitionCount > 0);

    return IsReleased;
}

// } [SECTION:IO]

#ifdef __cplusplus
}
#endif
