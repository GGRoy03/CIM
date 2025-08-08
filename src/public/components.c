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

    component *Component = QueryComponent(Id, QueryComponent_IsTreeRoot);
    window    *Window    = &Component->For.Window;

    // NOTE: This whole part is weird.
    if(Component->Type == Component_Invalid)
    {
        Component->Type = Component_Window;

        // Set the default state
        Window->IsClosed = false;

        // Set the retained data
        cim_point HeadAt = {500.0f, 500.0f};
        cim_f32   Width  = 300.0f;
        cim_f32   Height = 150.0f;
        Window->Head = AllocateQuad(HeadAt, Width, Height);

        cim_point BodyAt  = {500.0f, 650.0f};
        cim_f32   Width2  = 300.0f;
        cim_f32   Height2 = 400.0f;
        Window->Body = AllocateQuad(BodyAt, Width2, Height2);
    }

    cim_bit_field StyleUpFlags = Component->StyleUpdateFlags;

    if(Window->Style.HasBorders)
    {
        if(StyleUpFlags & StyleUpdate_BorderGeometry)
        {
            cim_point_node *Head   = Window->Head;
            cim_point_node *Body   = Window->Body;
            cim_u32         BWidth = Window->Style.BorderWidth; Cim_Assert(BWidth != 0);

            cim_f32   TopX    = Head->Value.x - Window->Style.BorderWidth;
            cim_f32   TopY    = Head->Value.y - Window->Style.BorderWidth;
            cim_point TopLeft = (cim_point){TopX, TopY};

            cim_f32 Width  = (Body->Prev->Value.x + Window->Style.BorderWidth) - TopX;
            cim_f32 Height = (Body->Prev->Value.y + Window->Style.BorderWidth) - TopY;

            if(!Window->Border)
            {
                Window->Border = AllocateQuad(TopLeft, Width, Height);
            }
            else
            {
                ReplaceQuad(Window->Border, TopLeft, Width, Height);
            }

            Component->StyleUpdateFlags &= ~Component->StyleUpdateFlags;
        }

        DrawQuad(Window->Border, Window->Style.BorderColor);
    }

    DrawQuad(Window->Head, Window->Style.HeadColor);
    DrawQuad(Window->Body, Window->Style.BodyColor);

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

bool
Cim_Button(const char *Id)
{
    cim_context *Ctx = CimContext; Cim_Assert(Ctx && "Forgot to initialize context?");

    component *Component = QueryComponent(Id, QueryComponent_NoFlags);
    button    *Button    = &Component->For.Button;

    if(Component->Type == Component_Invalid)
    {
        cim_point Pos = (cim_point){ Button->Style.Position.x, Button->Style.Position.y };
        Button->Body = AllocateQuad(Pos, Button->Style.Dimension.x, Button->Style.Dimension.y);

        // NOTE: temp
        Button->Style.BodyColor = (cim_vector4){1.0f, 0.0f, 0.5f, 1.0f};

        Component->Type = Component_Button;
    }

    // WARN: Messy. (I think we should simply hold a rect instead of points...)
    cim_rect Rect = { {Button->Body->Value.x, Button->Body->Value.y},
                      {Button->Body->Prev->Value.x, Button->Body->Prev->Value.y} };
    bool IsClicked = IsInsideRect(Rect) && IsMouseClicked(CimMouse_Left);

    DrawQuad(Button->Body, Button->Style.BodyColor);

    return IsClicked;
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
