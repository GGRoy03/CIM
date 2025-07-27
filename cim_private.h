#pragma once

#include "cim.h"

#include <string.h> // For memcpy

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ============================================================
// PRIVATE TYPE DEFINITIONS FOR  CIM. BY SECTION.
// -[SECTION] Hashing
// -[SECTION] Helpers
// -[SECTION] Primitives
// -[SECTION] Constraints
// -[SECTION] Commands
// -[SECTION] Widgets
// ============================================================
// ============================================================

// [SECTION:Hashing] {

#define FNV32Prime 0x01000193
#define FNV32Basis 0x811C9DC5

cim_u32 Cim_FindFirstBit32(cim_u32 Mask);
cim_u32 Cim_HashString(const char* String);

// } [SECTION:Hashing]

// [SECTION:Helpers] {

#define DECLARE_CIM_ARENA(Type, Name, FnName)                  \
typedef struct cim_arena_##Name                                \
{                                                              \
    Type  *Memory;                                             \
    size_t Size;                                               \
    size_t Capacity;                                           \
} cim_arena_##Name;                                            \
                                                               \
inline cim_arena_##Name                                        \
CimArena_##FnName_Begin(cim_u32 Count)                         \
{                                                              \
    cim_arena_##Name Arena;                                    \
    Arena.Capacity = Count;                                    \
    Arena.Size     = 0;                                        \
    Arena.Memory   = malloc(Count * sizeof(Type));             \
                                                               \
    return Arena;                                              \
}                                                              \
                                                               \
inline Type *                                                  \
CimArena_##FnName_Push(cim_u32 Count, cim_arena_##Name *Arena) \
{                                                              \
    Cim_Assert(Arena->Memory);                                 \
                                                               \
    if(Arena->Size + Count > Arena->Capacity)                  \
    {                                                          \
        Arena->Capacity *= 2;                                  \
                                                               \
        void *New = malloc(Arena->Capacity * sizeof(Type));    \
        if(!New)                                               \
        {                                                      \
            Cim_Assert(!"Malloc failure: OOM?");               \
            return NULL;                                       \
        }                                                      \
                                                               \
        free(Arena->Memory);                                   \
        Arena->Memory = New;                                   \
    }                                                          \
                                                               \
    Type *Ptr  = Arena->Memory + Arena->Size;                  \
    Arena->Size += Count;                                      \
                                                               \
    return Ptr;                                                \
}                                                              \
                                                               \
inline void                                                    \
CimArena_##FnName_Reset(cim_arena_##Name *Arena)               \
{                                                              \
    Arena->Size = 0;                                           \
}                                                              \
                                                               \
inline void                                                    \
CimArena_##FnName_End(cim_arena_##Name *Arena)                 \
{                                                              \
    if(Arena->Memory)                                          \
    {                                                          \
        free(Arena->Memory);                                   \
        Arena->Size     = 0;                                   \
        Arena->Capacity = 0;                                   \
    }                                                          \
}

DECLARE_CIM_ARENA(char   , byte, Byte)
DECLARE_CIM_ARENA(cim_u32, idx , Idx)

// } [SECTION:Helpers]


// [SECTION:Primitives] {

typedef struct cim_point
{
    cim_f32 x, y;
} cim_point;

typedef struct cim_window_state
{
    struct cim_point_node *Head;
    struct cim_point_node *Body;

    bool Closed;
} cim_window_state;

typedef struct cim_point_node
{
    cim_point              Value;
    struct cim_point_node *Prev;
    struct cim_point_node *Next;
} cim_point_node;

typedef struct cim_primitive_rings
{
    cim_point_node PointNodes[128];
    cim_u32        PointCount;
    cim_u32        PointNodesCapacity;
} cim_primitive_rings;

#define CimEmptyBucketTag 0x80
#define CimBucketGroupSize 16

cim_point_node *CimRing_PushQuad(cim_point p0, cim_point p1, cim_point p2, cim_point p3, cim_primitive_rings *Rings);

// } [SECTION:Primitives]

// [SECTION:Constraints] {

typedef enum CimConstraint_Type
{
    CimConstraint_Drag,

    CimConstraint_Count,
} CimConstraint_Type;

typedef struct cim_rect
{
    cim_vector2 Min;
    cim_vector2 Max;
} cim_rect;

typedef struct cim_context_drag
{
    cim_rect   BoundingBox;
    cim_point *FirstPoint;
} cim_context_drag;

typedef struct cim_constraint_manager
{
    cim_context_drag DragCtxs[4];
    cim_u32          RegDragCtxs;

} cim_constraint_manager;

// } [SECTION:Constraints]

// [SECTION:Commands] {

typedef struct cim_vtx_pos_tex_col
{
    cim_vector2 Pos;
    cim_vector2 Tex;
    cim_vector4 Col;
} cim_vtx_pos_tex_col;

typedef struct cim_command_batch
{
    cim_u32       VtxStride;
    cim_u32       IdxSize;
    cim_bit_field Features;
    cim_rect      ClippingRect;
} cim_command_batch;

typedef struct cim_command_buffer
{
    cim_arena_byte FrameVtx;
    cim_arena_idx  FrameIdx;

    cim_command_batch *Batches;
    cim_u32            BatchCount;
    cim_u32            BatchCapacity;

    bool ClippingRectChanged;
    bool FeatureStateChanged;
} cim_command_buffer;

void
CimCommand_PushQuad(cim_rect Rect, cim_vector4 Color);

// } [SECTION:Commands]

// [SECTION:Component] {

typedef enum CimComponent_Type
{
    CimComponent_Window,
} CimComponent_Type;

typedef struct cim_window_component
{
    cim_window_state State;

    struct cim_point_node *Head;
    struct cim_point_node *Body;
} cim_window_component;

typedef struct cim_component
{
    CimComponent_Type Type;
    union
    {
        cim_window_component Window;
    } For;
} cim_component;

typedef struct cim_component_entry
{
    char    Key[64];
    void   *Value;
    cim_u32 Hashed;
} cim_component_entry;

typedef struct cim_component_hashmap
{
    cim_u8              *Metadata;
    cim_component_entry *Buckets;
    cim_u32              GroupCount;
    bool                 IsInitialized;
} cim_component_hashmap;

// } [SECTION:Component]

// NOTE:
// 1) Maybe we wanna be able to give the user complete control over the command_buffer.
// 2) Maybe we wanna be able to have multiple command buffers (multi-threaded)

typedef struct cim_context
{
    void              *Backend;
    cim_command_buffer CmdBuffer;
    cim_io_inputs      Inputs;

    cim_primitive_rings PrimitiveRings;
} cim_context;

extern cim_context *CimContext;

#ifdef __cplusplus
}
#endif
