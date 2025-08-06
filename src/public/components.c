#include "cim_public.h"

#include "math.h"

#ifdef __cplusplus
extern "C" {
#endif

// TODO:
// 1) Change how we record constraints.
// 2) Allow for default positioning in the styling file.

bool
Cim_Window(const char *Id, cim_bit_field Flags)
{
    cim_context *Ctx = CimContext; Cim_Assert(Ctx && "Forgot to initialize context?");

    Ctx->CmdBuffer.ClippingRectChanged = true;

    cim_component       *Component = CimComponent_GetOrInsert(Id, &Ctx->ComponentStore);
    cim_window          *Window    = &Component->For.Window;
    cim_primitive_rings *Primitive = &Ctx->PrimitiveRings;
    cim_command_buffer  *CmdBuffer = &Ctx->CmdBuffer;

    // NOTE: This whole part is weird.
    if(Component->Type == CimComponent_Unknown)
    {
        Component->Type = CimComponent_Window;

        // Set the default state
        Window->IsClosed = false;

        // Set the retained data
        cim_point HeadAt = {500.0f, 500.0f};
        cim_u32   Width  = 300.0f;
        cim_u32   Height = 150.0f;
        Window->Head = CimPrimitive_PushQuad(HeadAt, Width, Height, Primitive);

        cim_point BodyAt  = {500.0f, 650.0f};
        cim_u32   Width2  = 300.0f;
        cim_u32   Height2 = 400.0f;
        Window->Body = CimPrimitive_PushQuad(BodyAt, Width2, Height2, Primitive);
    }

    // Draw the border around the window
    if(Window->Style.HasBorders)
    {
        if(!Window->Border)
        {
            cim_f32 BWidth = Window->Style.BorderWidth; Cim_Assert(BWidth != 0);

            cim_f32   TopX    = Window->Head->Value.x - Window->Style.BorderWidth;
            cim_f32   TopY    = Window->Head->Value.y - Window->Style.BorderWidth;
            cim_point TopLeft = (cim_point){TopX, TopY};

            cim_f32 Width  = (Window->Body->Prev->Value.x + Window->Style.BorderWidth) - TopX;
            cim_f32 Height = (Window->Body->Prev->Value.y + Window->Style.BorderWidth) - TopY;

            Window->Border = CimPrimitive_PushQuad(TopLeft, Width, Height, Primitive);
        }

       CimCommand_PushQuadEntry(Window->Border, Window->Style.BorderColor, CmdBuffer); 
    }

    // Draws the header + body
    CimCommand_PushQuadEntry(Window->Head, Window->Style.HeadColor, CmdBuffer);
    CimCommand_PushQuadEntry(Window->Body, Window->Style.BodyColor, CmdBuffer);

    // WARN: This will change soon.
    if(Flags & CimWindow_Draggable)
    {
        if(Window->Style.HasBorders)
        {
            cim_draggable Draggable = { {Window->Head, Window->Body, Window->Border }, 3};
            Drag[DragCount++] = Draggable;
        }
        else
        {
            cim_draggable Draggable = { {Window->Head, Window->Body }, 2};
            Drag[DragCount++] = Draggable;
        }
    }

    return true;
}

// WARN: This has not business being here.
void
Cim_EndFrame()
{
    cim_context *Ctx = CimContext; Cim_Assert(Ctx);

    cim_inputs *Inputs = &Ctx->Inputs;
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
