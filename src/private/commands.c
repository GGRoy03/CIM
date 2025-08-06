#include "cim_private.h"

#include <string.h> // memcpy

#ifdef __cplusplus
extern "C" {
#endif

cim_quad *
CimQuadStream_Read(cim_u32          ReadCount,
                   cim_quad_stream *Stream)
{
    if (Stream->ReadOffset + ReadCount > Stream->Capacity)
    {
        Cim_Assert(!"Attempted to read past the end of the quad stream.");
        return NULL;
    }

    cim_quad *ReadPointer = Stream->Source + Stream->ReadOffset;
    Stream->ReadOffset += ReadCount;

    return ReadPointer;
}

void
CimQuadStream_Write(cim_u32          WriteCount,
                    cim_quad        *Quads,
                    cim_quad_stream *Stream)
{
    if (Stream->WriteOffset + WriteCount > Stream->Capacity)
    {
        Stream->Capacity =
            Stream->Capacity ? Stream->Capacity + (Stream->Capacity >> 1) :
            16 * sizeof(cim_quad)
            ;

        while (Stream->Capacity < Stream->WriteOffset + WriteCount)
        {
            Stream->Capacity += (Stream->Capacity >> 1);
        }

        void *New = malloc(Stream->Capacity);
        if (!New)
        {
            Cim_Assert(!"Malloc failure: OOM?");
            return;
        }

        if (Stream->Source)
        {
            memcpy(New, Stream->Source, Stream->WriteOffset * sizeof(cim_quad));
            free(Stream->Source);
        }

        Stream->Source = New;
    }

    for (cim_u32 ReadIdx = 0; ReadIdx < WriteCount; ReadIdx++)
    {
        Stream->Source[Stream->WriteOffset++] = Quads[ReadIdx];
    }
}

void
    CimQuadStream_Reset(cim_quad_stream *Stream)
{
    Stream->ReadOffset = 0;
    Stream->WriteOffset = 0;
}

void
CimCommand_PushQuadEntry(cim_point_node     *Point,
                         cim_vector4         Color,
                         cim_command_buffer *CmdBuffer)
{
    cim_batch *Batch = NULL;
    if (CmdBuffer->ClippingRectChanged || CmdBuffer->FeatureStateChanged)
    {
        Batch = CimArena_Push(sizeof(cim_batch), &CmdBuffer->Batches);

        Batch->QuadsToRender = 0;

        CmdBuffer->ClippingRectChanged = false;
        CmdBuffer->FeatureStateChanged = false;
    }
    else
    {
        Batch = CimArena_GetLast(sizeof(cim_batch), &CmdBuffer->Batches);
    }

    Batch->QuadsToRender++;

    cim_quad Quad = { Point, Color };
    CimQuadStream_Write(1, &Quad, &CmdBuffer->Quads);
}

void
CimCommandStream_Write(cim_u32             WriteCount,
                       cim_draw_command   *Commands,
                       cim_command_stream *Stream)
{
    if (Stream->WriteOffset + WriteCount > Stream->Capacity)
    {
        Stream->Capacity = Stream->Capacity ? 
            Stream->Capacity + (Stream->Capacity >> CimDefault_CmdStreamGrowShift) :
            CimDefault_CmdStreamSize
        ;

        while (Stream->Capacity < Stream->WriteOffset + WriteCount)
        {
            Stream->Capacity += (Stream->Capacity >> CimDefault_CmdStreamGrowShift);
        }

        void *New = malloc(Stream->Capacity * sizeof(cim_draw_command));
        if (!New)
        { 
            Cim_Assert(!"Malloc failure: OOM?");
            return;
        }

        if (Stream->Source)
        { 
            memcpy(New, Stream->Source, Stream->WriteOffset * sizeof(cim_draw_command));
            free(Stream->Source); 
        }

        Stream->Source = New;
    }

    for (cim_u32 ReadIdx = 0; ReadIdx < WriteCount; ReadIdx++)
    {
        Stream->Source[Stream->WriteOffset++] = Commands[ReadIdx];
    }
}

cim_draw_command *
CimCommandStream_Read(cim_u32             ReadCount,
                      cim_command_stream *Stream)
{
    if (Stream->ReadOffset + ReadCount > Stream->Capacity)
    {
        Cim_Assert(!"Attempted to read past the end of the command stream.");
        return NULL;
    }

    cim_draw_command *ReadPointer = Stream->Source + Stream->ReadOffset;
    Stream->ReadOffset += ReadCount;

    return ReadPointer;
}

void
CimCommandStream_Reset(cim_command_stream *Stream)
{
    Stream->ReadOffset  = 0;
    Stream->WriteOffset = 0;
}

void
CimCommand_BuildDrawData(cim_command_buffer *CmdBuffer)
{
    const cim_u32 QuadVtxCount = 4;
    const cim_u32 QuadIdxCount = 6;
    const size_t  QuadVtxSize  = QuadVtxCount * sizeof(cim_vtx);
    const size_t  QuadIdxSize  = QuadIdxCount * sizeof(cim_u32);

    cim_u32 BatchCount = CimArena_GetCount(sizeof(cim_batch), &CmdBuffer->Batches);
    for (cim_u32 BatchIdx = 0; BatchIdx < BatchCount; BatchIdx++)
    {
        cim_batch *Batch = (cim_batch*)CmdBuffer->Batches.Memory + BatchIdx;

        cim_draw_command Command = {0};
        Command.VtxOffset = CmdBuffer->FrameVtx.At;
        Command.IdxOffset = CmdBuffer->FrameIdx.At;
        Command.Features  = Batch->Features;

        cim_quad *QuadStream = CimQuadStream_Read(Batch->QuadsToRender, &CmdBuffer->Quads);
        for (cim_u32 QuadIdx = 0; QuadIdx < Batch->QuadsToRender; QuadIdx++)
        {
            cim_quad Quad = QuadStream[QuadIdx];

            cim_u32 *Idx = CimArena_Push(QuadIdxSize, &CmdBuffer->FrameIdx);
            if (Idx)
            {
                Idx[0] = Command.VtxCount + 0;
                Idx[1] = Command.VtxCount + 2;
                Idx[2] = Command.VtxCount + 1;

                Idx[3] = Command.VtxCount + 2;
                Idx[4] = Command.VtxCount + 3;
                Idx[5] = Command.VtxCount + 1;

                Command.IdxCount += QuadIdxCount;
            }

            cim_vtx *Vtx = CimArena_Push(QuadVtxSize, &CmdBuffer->FrameVtx);
            if (Vtx)
            {
                cim_point   First = Quad.First->Value;
                cim_point   Last  = Quad.First->Prev->Value;
                cim_vector4 Color = Quad.Color;
                cim_rect    Rect = { {First.x, First.y}, {Last.x, Last.y} };

                Vtx[0] = (cim_vtx){{Rect.Min.x,Rect.Min.y},{0,0},{Color.x,Color.y,Color.z,Color.w}};
                Vtx[1] = (cim_vtx){{Rect.Min.x,Rect.Max.y},{0,0},{Color.x,Color.y,Color.z,Color.w}};
                Vtx[2] = (cim_vtx){{Rect.Max.x,Rect.Min.y},{0,0},{Color.x,Color.y,Color.z,Color.w}};
                Vtx[3] = (cim_vtx){{Rect.Max.x,Rect.Max.y},{0,0},{Color.x,Color.y,Color.z,Color.w}};

                Command.VtxCount += QuadVtxCount;
            }
        }

        CimCommandStream_Write(1, &Command, &CmdBuffer->Commands);
    }
}

#ifdef __cplusplus
}
#endif
