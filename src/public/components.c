#include "cim_public.h"

#ifdef __cplusplus
extern "C" {
#endif

static bool Cim_Window(const char *Id, cim_vector4 Color, cim_bit_field Flags)
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
        cim_point HeadAt = {500.0f, 500.0f};
        cim_u32   Width  = 300.0f;
        cim_u32   Height = 150.0f;
        Window->Head = CimPrimitive_PushQuad(HeadAt, Width, Height, &Ctx->PrimitiveRings);

        cim_point BodyAt  = {500.0f, 650.0f};
        cim_u32   Width2  = 300.0f;
        cim_u32   Height2 = 400.0f;
        Window->Body = CimPrimitive_PushQuad(BodyAt, Width2, Height2, &Ctx->PrimitiveRings);
    }

    if(Flags & CimWindow_Draggable)
    {
        cim_draggable Draggable = { {Window->Head, Window->Body }, 2};
        Drag[DragCount++] = Draggable;
    }

    CimCommand_PushQuadEntry(Window->Head, Color, &CimContext->CmdBuffer);

    if(!Window->IsClosed)
    {
        cim_vector4 BodyColor = { 0.3f, 0.3f, 0.3f, 1.0f };
        CimCommand_PushQuadEntry(Window->Body, BodyColor, &Ctx->CmdBuffer);
    }

    return true;
}

#ifdef __cplusplus
}
#endif
