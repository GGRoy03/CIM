#include "cim_private.h"

#ifdef __cplusplus
extern "C" {
#endif

bool 
CimInput_IsMouseDown(CimMouse_Button MouseButton, cim_inputs *Inputs)
{
    bool IsDown = Inputs->MouseButtons[MouseButton].EndedDown;

    return IsDown;
}

bool 
CimInput_IsMouseReleased(CimMouse_Button MouseButton, cim_inputs *Inputs)
{
    cim_button_state *State = &Inputs->MouseButtons[MouseButton];
    bool IsReleased = (!State->EndedDown) && (State->HalfTransitionCount > 0);

    return IsReleased;
}

bool
CimInput_IsMouseClicked(CimMouse_Button MouseButton, cim_inputs *Inputs)
{
    cim_button_state *State = &Inputs->MouseButtons[MouseButton];
    bool IsClicked = (State->EndedDown) && (State->HalfTransitionCount > 0);

    return IsClicked;
}

cim_i32 
CimInput_GetMouseDeltaX(cim_inputs *Inputs)
{
    cim_i32 DeltaX = Inputs->MouseDeltaX;

    return DeltaX;
}

cim_i32 
CimInput_GetMouseDeltaY(cim_inputs *Inputs)
{
    cim_i32 DeltaY = Inputs->MouseDeltaY;

    return DeltaY;
}

cim_vector2 
CimInput_GetMousePosition(cim_inputs *Inputs)
{
    cim_vector2 Position = (cim_vector2){(cim_f32)Inputs->MouseX, (cim_f32)Inputs->MouseY};

    return Position;
}

#ifdef __cplusplus
}
#endif
