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

typedef struct cim_arena
{
    void  *Memory;
    size_t At;
    size_t Capacity;
} cim_arena;

cim_arena
CimArena_Begin(size_t Size);

void *
CimArena_Push(size_t Size, cim_arena *Arena);

void *
CimArena_GetLast(size_t TypeSize, cim_arena *Arena);

cim_u32
CimArena_GetCount(size_t TypeSize, cim_arena *Arena);

void
CimArena_Reset(cim_arena *Arena);

void
CimArena_End(cim_arena *Arena);

// } [SECTION:Helpers]

// [SECTION:Primitives] {

typedef struct cim_point
{
    cim_f32 x, y;
} cim_point;

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

cim_point_node *
CimPoint_PushQuad(cim_point p0, cim_point p1, cim_point p2, cim_point p3);

// } [SECTION:Primitives]

// [SECTION:Constraints] {

typedef struct cim_rect
{
    cim_vector2 Min;
    cim_vector2 Max;
} cim_rect;

typedef enum CimConstraint_Type
{
    CimConstraint_Drag,

    CimConstraint_Count,
} CimConstraint_Type;

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

typedef struct cim_vtx
{
    cim_vector2 Pos;
    cim_vector2 Tex;
    cim_vector4 Col;
} cim_vtx;

typedef struct cim_quad
{
    cim_point_node *First;
    cim_vector4     Color;
} cim_quad;

typedef struct cim_batch
{
    cim_u32 QuadsToRender;

    cim_bit_field Features;
    cim_rect      ClippingRect;
} cim_batch;

typedef struct cim_quad_stream
{
    cim_quad *Source;
    cim_u32   Capacity;

    cim_u32 WriteOffset;
    cim_u32 ReadOffset;
} cim_quad_stream;

typedef struct cim_draw_command
{
    cim_u32 VtxOffset;
    cim_u32 IdxOffset;
    cim_u32 IdxCount;
    cim_u32 VtxCount;

    cim_rect      ClippingRect;
    cim_bit_field Features;
} cim_draw_command;

typedef struct cim_command_stream
{
    cim_draw_command *Source;
    cim_u32           Capacity;

    cim_u32 WriteOffset;
    cim_u32 ReadOffset;
} cim_command_stream;

typedef struct cim_command_buffer
{
    // Still unsure about streams. I might want special functionalities for types...
    cim_quad_stream    Quads;
    cim_command_stream Commands;

    cim_arena FrameVtx;
    cim_arena FrameIdx;

    // NOTE: Still really unclear what we really want here.
    cim_arena Batches;

    bool ClippingRectChanged;
    bool FeatureStateChanged;
} cim_command_buffer;

// Streams

cim_quad *
CimQuadStream_Read(cim_u32          Count,
                   cim_quad_stream *Stream);

void
CimQuadStream_Write(cim_u32          Count,
                    cim_quad        *Quads,
                    cim_quad_stream *Stream);
void
CimQuadStream_Reset(cim_quad_stream *Stream);

cim_command_stream *
CimCommandStream_Read(cim_u32             ReadCount,
                      cim_command_stream *Stream);

void 
CimCommandStream_Write(cim_u32             WriteCount,
                       cim_draw_command   *Commands,
                       cim_command_stream *Stream);
void
CimCommandStream_Reset(cim_command_stream *Stream);

void
CimCommand_PushQuadEntry(cim_point_node *Point, cim_vector4 Color);

// TEMP
void
CimCommand_BuildSceneGeometry();

// } [SECTION:Commands]

// [SECTION:Component] {

typedef enum CimComponent_Type
{
    CimComponent_Unknown,
    CimComponent_Window,
} CimComponent_Type;

typedef struct cim_window
{
    bool IsClosed;

    struct cim_point_node *Head;
    struct cim_point_node *Body;
} cim_window;

typedef struct cim_component
{
    bool IsInitialized;

    CimComponent_Type Type;
    union
    {
        cim_window Window;
    } For;
} cim_component;

typedef struct cim_component_entry
{
    char          Key[64];
    cim_component Value;
    cim_u32       Hashed;
} cim_component_entry;

typedef struct cim_component_hashmap
{
    cim_u8              *Metadata;
    cim_component_entry *Buckets;
    cim_u32              GroupCount;
    bool                 IsInitialized;
} cim_component_hashmap;

cim_component *
CimComponent_GetOrInsert(const char *Key, cim_component_hashmap *Hashmap);

// } [SECTION:Component]

typedef struct cim_context
{
    void              *Backend;
    cim_command_buffer CmdBuffer;
    cim_io_inputs      Inputs;

    // Subject to change soon
    cim_component_hashmap ComponentStore;

    // Rework incoming
    cim_primitive_rings PrimitiveRings;
} cim_context;

extern cim_context *CimContext;

#ifdef __cplusplus
}
#endif
