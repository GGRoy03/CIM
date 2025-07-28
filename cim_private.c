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

    void *Last = (char *)Arena->Memory + Arena->At;

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
CimCommand_PushQuad(cim_rect Rect, cim_f32 *Color)
{
    cim_context        *Context   = CimContext;          Cim_Assert(Context);
    cim_command_buffer *CmdBuffer = &Context->CmdBuffer;

    cim_command_batch *Batch = NULL;
    if(CmdBuffer->ClippingRectChanged || CmdBuffer->FeatureStateChanged)
    {
        Batch = CimArena_Push(sizeof(cim_command_batch), &CmdBuffer->Batches);

        Batch->VtxOffset = CmdBuffer->FrameVtx.At;
        Batch->IdxOffset = CmdBuffer->FrameIdx.At;

        CmdBuffer->ClippingRectChanged = false;
        CmdBuffer->FeatureStateChanged = false;
    }
    else
    {
        Batch = CimArena_GetLast(sizeof(cim_command_batch), &CmdBuffer->Batches);
    }

    cim_vtx_pos_tex_col *Vtx = 
        CimArena_Push(4 * sizeof(cim_vtx_pos_tex_col), &CmdBuffer->FrameVtx);

    if(Vtx)
    {
        Vtx[0].Pos = (cim_vector2){Rect.Min.x, Rect.Min.y};
        Vtx[0].Tex = (cim_vector2){0.0f, 0.0f};
        Vtx[0].Col = (cim_vector4){Color[0], Color[1], Color[2], Color[3]};

        Vtx[1].Pos = (cim_vector2){Rect.Min.x, Rect.Max.y};
        Vtx[1].Tex = (cim_vector2){0.0f, 0.0f};
        Vtx[1].Col = (cim_vector4){Color[0], Color[1], Color[2], Color[3]};

        Vtx[2].Pos = (cim_vector2){Rect.Max.x, Rect.Min.y};
        Vtx[2].Tex = (cim_vector2){0.0f, 0.0f};
        Vtx[2].Col = (cim_vector4){Color[0], Color[1], Color[2], Color[3]};

        Vtx[3].Pos = (cim_vector2){Rect.Max.x, Rect.Max.y};
        Vtx[3].Tex = (cim_vector2){0.0f, 0.0f};
        Vtx[3].Col = (cim_vector4){Color[0], Color[1], Color[2], Color[3]};
    }

    cim_u32  IdxCount = 6;
    cim_u32 *Indices  = CimArena_Push(IdxCount * sizeof(cim_u32), &CmdBuffer->FrameIdx);

    if(Indices)
    {
        Indices[0] = 0;
        Indices[1] = 1;
        Indices[2] = 2;

        Indices[3] = 1;
        Indices[4] = 3;
        Indices[5] = 2;

        Batch->IdxCount += IdxCount;
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
