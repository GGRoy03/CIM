#include "cim_private.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ============================================================
// PRIVATE IMPLEMENTATION FOR CIM. BY SECTION.
// -[SECTION:Hashing]
// -[SECTION:Primitives]
// -[SECTION:Conraints]
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

// [SECTION:Primitives] {

// NOTE: This is a direct copy of the get routine. The order of operation should be
// changed/overall data flow.

void CimMap_AddStateEntry(const char *Key, cim_state_node *Value, cim_state_hashmap *Hashmap)
{
    cim_u32 ProbeCount  = 0;
    cim_u32 HashedValue = Cim_HashString(Key);
    cim_u32 GroupIndex  = HashedValue & (Hashmap->GroupCount - 1);

    while (true)
    {
        cim_u8 *Meta = Hashmap->Metadata + (GroupIndex * CimBucketGroupSize);
        cim_u8  Tag = (HashedValue & 0x7F);

        __m128i MetaVector = _mm_loadu_si128((__m128i *)Meta);
        __m128i TagVector  = _mm_set1_epi8(Tag);

        cim_i32 Mask = _mm_movemask_epi8(_mm_cmpeq_epi8(MetaVector, TagVector));

        while (Mask)
        {
            cim_u32 Lane  = Cim_FindFirstBit32(Mask);
            cim_u32 Index = (GroupIndex * CimBucketGroupSize) + Lane;

            cim_state_entry *Entry = Hashmap->Buckets + Index;
            if(strcmp(Entry->Key, Key) == 0)
            {
                return;
            }

            Mask &= Mask - 1;
        }

        __m128i EmptyVector = _mm_set1_epi8(CimEmptyBucketTag);
        cim_i32 MaskEmpty   = _mm_movemask_epi8(_mm_cmpeq_epi8(MetaVector, EmptyVector));

        if (MaskEmpty)
        {
            cim_i32 Lane  = Cim_FindFirstBit32(MaskEmpty);
            cim_u32 Index = (GroupIndex * CimBucketGroupSize) + Lane;

            cim_state_entry *Entry = Hashmap->Buckets + Index;
            strcpy_s(Entry->Key, sizeof(Entry->Key), Key);
            Entry->Value = Value;

            Meta[Lane] = Tag;

            return;
        }

        ProbeCount++;
        GroupIndex = (GroupIndex + (ProbeCount * ProbeCount)) & (Hashmap->GroupCount - 1);
    }
}

cim_state_node* CimMap_GetStateValue(const char *Key, cim_state_hashmap *Hashmap)
{
    if(!Hashmap->IsInitialized)
    {
        Hashmap->GroupCount = 32;

        cim_u32 BucketCount = Hashmap->GroupCount * CimBucketGroupSize;

        Hashmap->Buckets  = malloc(BucketCount * sizeof(cim_state_entry));
        Hashmap->Metadata = malloc(BucketCount * sizeof(cim_u8));

        if (!Hashmap->Buckets || !Hashmap->Metadata)
        {
            return NULL;
        }

        memset(Hashmap->Buckets, 0, BucketCount * sizeof(cim_state_entry));
        memset(Hashmap->Metadata, CimEmptyBucketTag, BucketCount * sizeof(cim_u8));

        Hashmap->IsInitialized = true;
    }

    cim_u32 ProbeCount  = 0;
    cim_u32 HashedValue = Cim_HashString(Key);
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

            cim_state_entry *Entry = Hashmap->Buckets + Index;
            if(strcmp(Entry->Key, Key) == 0)
            {
                return Entry->Value;
            }

            Mask &= Mask - 1;
        }

        __m128i EmptyVector = _mm_set1_epi8(CimEmptyBucketTag);
        cim_i32 MaskEmpty   = _mm_movemask_epi8(_mm_cmpeq_epi8(MetaVector, EmptyVector));

        if (MaskEmpty)
        {
            return NULL;
        }

        ProbeCount++;
        GroupIndex = (GroupIndex + (ProbeCount * ProbeCount)) & (Hashmap->GroupCount - 1);
    }
}

cim_state_node * 
CimRing_AddStateNode(cim_ui_state State, cim_primitive_rings *Rings)
{
    cim_state_node *Result = Rings->StateNodes + Rings->AllocatedStateNodes++;

    Result->State  = State;
    Result->Parent = NULL;
    Result->Child  = NULL;

    return Result;
}

cim_point_node *
CimRing_PushQuad(cim_point p0, cim_point p1, cim_point p2, cim_point p3, cim_primitive_rings *Rings)
{
    cim_point_node *Point0 = Rings->PointNodes + Rings->PointCount++;
    cim_point_node *Point1 = Rings->PointNodes + Rings->PointCount++;
    cim_point_node *Point2 = Rings->PointNodes + Rings->PointCount++;
    cim_point_node *Point3 = Rings->PointNodes + Rings->PointCount++;

    Point0->Prev = Point3;
    Point0->Next = Point1;

    Point1->Prev = Point1;
    Point1->Next = Point2;

    Point2->Prev = Point1;
    Point2->Next = Point3;

    Point3->Prev = Point2;
    Point3->Next = Point0;

    return Point0;
};

// } [SECTION:Primitives]

// [SECTION:Constraints] {

void CimConstraint_Register(CimConstraint_Type ConstType, void *Context)
{
    cim_context            *Ctx     = CimContext;
    cim_constraint_manager *Manager = &Ctx->ConstraintManager;

    switch(ConstType)
    {

    case CimConstraint_Drag:
    {
        cim_context_drag *WritePointer = &Manager->DragCtxs[Manager->RegDragCtxs++];
        memcpy(WritePointer, Context, sizeof(cim_context_drag));
    } break;

    default:
    {

    } break;

    }
}

void CimConstraint_SolveAll()
{
    cim_context            *Ctx     = CimContext;
    cim_constraint_manager *Manager = &Ctx->ConstraintManager;

    bool    MouseDown   = Cim_IsMouseDown(CimMouse_Left);
    cim_f32 MouseDeltaX = Cim_GetMouseDeltaX();
    cim_f32 MouseDeltaY = Cim_GetMouseDeltaY();

    if(MouseDown)
    {
        cim_f32 DragSpeed = 0.5f;
        cim_f32 DragX     = MouseDeltaX * DragSpeed;
        cim_f32 DragY     = MouseDeltaY * DragSpeed;

        for(cim_u32 CIdx = 0; CIdx < Manager->RegDragCtxs; CIdx++)
        {
        }
    }
    Manager->RegDragCtxs = 0;
}

// } [SECTION:Constraints]

// [SECTION:Commands] {

void CimCommand_Push(cim_command_header *Header, void *Payload)
{
    cim_context        *Ctx    = CimContext;
    cim_command_buffer *Buffer = &Ctx->Commands;

    cim_u32 Needed = sizeof(cim_command_header) + Header->Size;
    if(Buffer->Size + Needed <= Buffer->Capacity)
    {
        memcpy(Buffer->Base + Buffer->Size, Header, sizeof(cim_command_header));
        Buffer->Size += sizeof(cim_command_header);

        memcpy(Buffer->Base + Buffer->Size, Payload, Header->Size);
        Buffer->Size += Header->Size;
    }
    else
    {
        abort();
    }
}

void CimCommand_Quad(cim_point_node *FirstPoint, cim_vector4 Color)
{
    cim_command_header Header;
    Header.Type = CimCommand_Quad;
    Header.Size = sizeof(cim_payload_quad);

    cim_payload_quad Payload;
    Payload.Point = FirstPoint;
    Payload.Color = Color;

    CimCommand_Push(&Header, &Payload);
}

// } [SECTION:Commands]

cim_context *CimContext;

#ifdef __cplusplus
}
#endif
