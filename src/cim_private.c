#include "cim_private.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ============================================================
// PRIVATE IMPLEMENTATION FOR CIM. BY SECTION.
// -[SECTION:Hashing]
// -[SECTION:Helpers]
// -[SECTION:Primitives]
// -[SECTION:Commands]
// -[SECTION:Components]
// ============================================================
// ============================================================

// [SECTION:Hashing] {

cim_u32 Cim_FindFirstBit32(cim_u32 Mask)
{
    unsigned long Index = 0;
    _BitScanForward(&Index, Mask);
    return (cim_u32)Index;
}

cim_u32 Cim_HashString(const char* String)
{
    if (!String)
    {
        Cim_Assert(!"Cannot hash empty string.");
        return 0;
    }

    cim_u32 Result = FNV32Basis;
    cim_u8  Character;

    while ((Character = *String++))
    {
        Result = FNV32Prime * (Result ^ Character);
    }

    return Result;
}

// } [SECTION:Hashing]

// [SECTION:Helpers] {

cim_arena
CimArena_Begin(size_t Size)
{
    cim_arena Arena;
    Arena.Capacity = Size;
    Arena.At       = 0;
    Arena.Memory   = malloc(Size);

    return Arena;
}

void *
CimArena_Push(size_t Size, cim_arena *Arena)
{
    if(Arena->At + Size > Arena->Capacity)
    {
        Arena->Capacity = Arena->Capacity ? Arena->Capacity * 2 : 1024;

        while (Arena->Capacity < Arena->At + Size)
        {
            Arena->Capacity += (Arena->Capacity >> 1);
        }

        void *New = malloc(Arena->Capacity);
        if(!New)
        {
            Cim_Assert(!"Malloc failure: OOM?");
            return NULL;
        }
        else
        {
            memset((char*)New + Arena->At, 0, Arena->Capacity - Arena->At);
        }

        if (Arena->Memory)
        {
            memcpy(New, Arena->Memory, Arena->At);
            free(Arena->Memory);
        }

        Arena->Memory = New;
    }

    void *Ptr  = (char*)Arena->Memory + Arena->At;
    Arena->At += Size;

    return Ptr;
}

void *
CimArena_GetLast(size_t TypeSize, cim_arena *Arena)
{
    Cim_Assert(Arena->Memory);

    void *Last = (char *)Arena->Memory + (Arena->At - TypeSize);

    return Last;
}

cim_u32
CimArena_GetCount(size_t TypeSize, cim_arena *Arena)
{
    Cim_Assert(Arena->Memory);

    cim_u32 Count = Arena->At / TypeSize;

    return Count;
}

void 
CimArena_Reset(cim_arena *Arena)
{
    Arena->At = 0;
}

void 
CimArena_End(cim_arena *Arena)
{
    if(Arena->Memory)
    {
        free(Arena->Memory);

        Arena->Memory   = NULL;
        Arena->At       = 0;
        Arena->Capacity = 0;
    }
}

// } [SECTION:Helpers]

// [SECTION:Primitives] {
// NOTE: To store a quad, do we really need all of it's points? We are gonna build
// a rect anyway...

cim_point_node *
CimPoint_PushQuad(cim_point p0, cim_point p1, cim_point p2, cim_point p3)
{
    Cim_Assert(CimContext);

    cim_primitive_rings *Rings = &CimContext->PrimitiveRings;

    cim_point_node *Point0 = Rings->PointNodes + Rings->PointCount++;
    cim_point_node *Point1 = Rings->PointNodes + Rings->PointCount++;
    cim_point_node *Point2 = Rings->PointNodes + Rings->PointCount++;
    cim_point_node *Point3 = Rings->PointNodes + Rings->PointCount++;

    Point0->Value = p0;
    Point0->Prev  = Point3;
    Point0->Next  = Point1;

    Point1->Value = p1;
    Point1->Prev  = Point1;
    Point1->Next  = Point2;

    Point2->Value = p2;
    Point2->Prev  = Point1;
    Point2->Next  = Point3;

    Point3->Value = p3;
    Point3->Prev  = Point2;
    Point3->Next  = Point0;

    return Point0;
};

// } [SECTION:Primitives]

// [SECTION:Commands] {
// WARN:
// 1) Still haven't fixed the batch memory allocation problem.

void
CimCommand_PushQuadEntry(cim_point_node *Point, cim_vector4 Color)
{
    cim_context        *Context   = CimContext;          Cim_Assert(Context);
    cim_command_buffer *CmdBuffer = &Context->CmdBuffer;

    cim_batch *Batch = NULL;
    if(CmdBuffer->ClippingRectChanged || CmdBuffer->FeatureStateChanged)
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

    cim_quad Quad;
    Quad.First = Point;
    Quad.Color = Color;
    CimQuadStream_Write(1, &Quad, &CmdBuffer->Quads);
}

cim_quad *
CimQuadStream_Read(cim_u32          ReadCount, 
                   cim_quad_stream *Stream)
{
    if(Stream->ReadOffset + ReadCount > Stream->Capacity)
    {
        Cim_Assert(!"Attempted to read past the end of the quad stream.\n");
        return NULL;
    }

    cim_quad *ReadPointer = Stream->Source + Stream->ReadOffset;

    Stream->ReadOffset += ReadCount;

    return ReadPointer;
}

void
CimQuadStream_Reset(cim_quad_stream *Stream)
{
    Stream->ReadOffset  = 0;
    Stream->WriteOffset = 0;
}

void CimQuadStream_Write(cim_u32          WriteCount,
                         cim_quad        *Quads,
                         cim_quad_stream *Stream)
{
    if(Stream->WriteOffset + WriteCount > Stream->Capacity)
    {
        Stream->Capacity =
            Stream->Capacity ? Stream->Capacity + (Stream->Capacity >> 1) :
            16 * sizeof(cim_quad) 
        ;

        while(Stream->Capacity < Stream->WriteOffset + WriteCount)
        {
            Stream->Capacity += (Stream->Capacity >> 1);
        }

        void *New = malloc(Stream->Capacity);
        if(!New)
        {
            Cim_Assert(!"Malloc failure: OOM?");
            return NULL;
        }

        if (Stream->Source)
        {
            memcpy(New, Stream->Source, Stream->WriteOffset * sizeof(cim_quad));
            free(Stream->Source);
        }

        Stream->Source = New;
    }

    for(cim_u32 ReadIdx = 0; ReadIdx < WriteCount; ReadIdx++)
    {
        Stream->Source[Stream->WriteOffset++] = Quads[ReadIdx];
    }
}

void 
CimCommandStream_Write(cim_u32             WriteCount,
                       cim_draw_command   *Commands,
                       cim_command_stream *Stream)
{
    if(Stream->WriteOffset + WriteCount > Stream->Capacity)
    {
        Stream->Capacity = Stream->Capacity ? 
            Stream->Capacity + (Stream->Capacity >> 1) :
            16 * sizeof(cim_draw_command)
        ;

        while(Stream->Capacity < Stream->WriteOffset + WriteCount)
        {
            Stream->Capacity += (Stream->Capacity >> 1);
        }

        void *New = malloc(Stream->Capacity);
        if(!New)
        {
            Cim_Assert(!"Malloc failure: OOM?");
            return NULL;
        }

        if (Stream->Source)
        {
            memcpy(New, Stream->Source, Stream->WriteOffset * sizeof(cim_draw_command));
            free(Stream->Source);
        }

        Stream->Source = New;
    }

    for(cim_u32 CmdIdx = 0; CmdIdx < WriteCount; CmdIdx++)
    {
        Stream->Source[Stream->WriteOffset++] = Commands[CmdIdx];
    }
}

cim_command_stream *
CimCommandStream_Read(cim_u32 ReadCount, cim_command_stream *Stream)
{
    if(Stream->ReadOffset + ReadCount > Stream->Capacity)
    {
        Cim_Assert(!"Attempted to read past the end of the command stream.\n");
        return NULL;
    }

    cim_draw_command *Command = Stream->Source + Stream->ReadOffset;

    Stream->ReadOffset += ReadCount;

    return Command;
}

void
CimCommandStream_Reset(cim_command_stream *Stream)
{
    Stream->ReadOffset  = 0;
    Stream->WriteOffset = 0;
}

void 
CimCommand_BuildSceneGeometry()
{
    cim_context *Ctx = CimContext; Cim_Assert(Ctx);

    cim_command_buffer *CmdBuffer = &Ctx->CmdBuffer;

    // WARN: This kind of depends on the stride (The size part). Prototyping.
    const cim_u32 QuadVtxCount = 4;
    const cim_u32 QuadIdxCount = 6;
    const size_t  QuadVtxSize  = QuadVtxCount * sizeof(cim_vtx);
    const size_t  QuadIdxSize  = QuadIdxCount * sizeof(cim_u32);

    cim_u32 BatchCount = CimArena_GetCount(sizeof(cim_batch), &CmdBuffer->Batches);
    for(cim_u32 BatchIdx = 0; BatchIdx < BatchCount; BatchIdx++)
    {
        cim_batch *Batch = (cim_batch*)CmdBuffer->Batches.Memory + BatchIdx;

        cim_draw_command Command = {0};
        Command.VtxOffset = CmdBuffer->FrameVtx.At;
        Command.IdxOffset = CmdBuffer->FrameIdx.At;
        Command.Features  = Batch->Features;

        cim_u32   QuadCount  = Batch->QuadsToRender;
        cim_quad *QuadStream = CimQuadStream_Read(QuadCount, &CmdBuffer->Quads);

        for(cim_u32 QuadIdx = 0; QuadIdx < Batch->QuadsToRender; QuadIdx++)
        {
            cim_quad Quad = QuadStream[QuadIdx];

            cim_u32 *Idx = CimArena_Push(QuadIdxSize, &CmdBuffer->FrameIdx);
            if(Idx)
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
            if(Vtx)
            {
                cim_point   First = Quad.First->Value;
                cim_point   Last  = Quad.First->Prev->Value;
                cim_vector4 Color = Quad.Color;
                cim_rect    Rect  = (cim_rect){First.x, First.y, Last.x, Last.y};

                Vtx[0].Pos = (cim_vector2){Rect.Min.x, Rect.Min.y};
                Vtx[0].Tex = (cim_vector2){0.0f, 0.0f};
                Vtx[0].Col = (cim_vector4){Color.x, Color.y, Color.z, Color.w};

                Vtx[1].Pos = (cim_vector2){Rect.Min.x, Rect.Max.y};
                Vtx[1].Tex = (cim_vector2){0.0f, 0.0f};
                Vtx[1].Col = (cim_vector4){Color.x, Color.y, Color.z, Color.w};

                Vtx[2].Pos = (cim_vector2){Rect.Max.x, Rect.Min.y};
                Vtx[2].Tex = (cim_vector2){0.0f, 0.0f};
                Vtx[2].Col = (cim_vector4){Color.x, Color.y, Color.z, Color.w};

                Vtx[3].Pos = (cim_vector2){Rect.Max.x, Rect.Max.y };
                Vtx[3].Tex = (cim_vector2){0.0f, 0.0f};
                Vtx[3].Col = (cim_vector4){Color.x, Color.y, Color.z, Color.w};

                Command.VtxCount += QuadVtxCount;
            }
        }

        CimCommandStream_Write(1, &Command, &CmdBuffer->Commands);
    }
}

// } [SECTION:Commands]

// [SECTION:Components] {

cim_component *
CimComponent_GetOrInsert(const char *Key, cim_component_hashmap *Hashmap)
{
    if(!Hashmap->IsInitialized)
    {
        Hashmap->GroupCount = 32;

        cim_u32 BucketCount = Hashmap->GroupCount * CimBucketGroupSize;

        Hashmap->Buckets  = malloc(BucketCount * sizeof(cim_component_entry));
        Hashmap->Metadata = malloc(BucketCount * sizeof(cim_u8));

        if (!Hashmap->Buckets || !Hashmap->Metadata)
        {
            return NULL;
        }

        memset(Hashmap->Buckets, 0, BucketCount * sizeof(cim_component_entry));
        memset(Hashmap->Metadata, CimEmptyBucketTag, BucketCount * sizeof(cim_u8));

        Hashmap->IsInitialized = true;
    }

    cim_u32 ProbeCount  = 0;
    cim_u32 HashedValue = 0;
    cim_u32 GroupIndex  = HashedValue & (Hashmap->GroupCount - 1);

    while (true)
    {
        cim_u8 *Meta = Hashmap->Metadata + (GroupIndex * CimBucketGroupSize);
        cim_u8  Tag  = (HashedValue & 0x7F);

        __m128i MetaVector = _mm_loadu_si128((__m128i *)Meta);
        __m128i TagVector  = _mm_set1_epi8(Tag);

        cim_i32 Mask = _mm_movemask_epi8(_mm_cmpeq_epi8(MetaVector, TagVector));

        while (Mask)
        {
            cim_u32 Lane  = Cim_FindFirstBit32(Mask);
            cim_u32 Index = (GroupIndex * CimBucketGroupSize) + Lane;

            cim_component_entry *Entry = Hashmap->Buckets + Index;
            if(strcmp(Entry->Key, Key) == 0)
            {
                return &Entry->Value;
            }

            Mask &= Mask - 1;
        }

        __m128i EmptyVector = _mm_set1_epi8(CimEmptyBucketTag);
        cim_i32 MaskEmpty   = _mm_movemask_epi8(_mm_cmpeq_epi8(MetaVector, EmptyVector));

        if (MaskEmpty)
        {
            cim_u32 Lane  = Cim_FindFirstBit32(MaskEmpty);
            cim_u32 Index = (GroupIndex * CimBucketGroupSize) + Lane;

            cim_component_entry *Entry = Hashmap->Buckets + Index;
            strcpy_s(Entry->Key, sizeof(Entry->Key), Key);
            Entry->Key[sizeof(Entry->Key) - 1] = 0;

            Meta[Lane] = Tag;

            return &Entry->Value;
        }

        ProbeCount++;
        GroupIndex = (GroupIndex + (ProbeCount * ProbeCount)) & (Hashmap->GroupCount - 1);
    }
}

// } [SECTION:Components]

cim_context *CimContext;

#ifdef __cplusplus
}
#endif
