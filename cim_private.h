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

// [SECTION:Primitives] {

typedef struct cim_primitive_point
{
    cim_f32 x, y;
} cim_point;

typedef enum CimUIState_Type
{
    CimUIState_Window,
} CimUIState_Type;

typedef struct cim_window_state
{
    struct cim_point_node *Head;
    struct cim_command    *HeadCmd;

    struct cim_point_node *Body;
    struct cim_command    *BodyCmd;

    bool Closed;
} cim_window_state;

typedef struct cim_primitive_ui_state
{
    CimUIState_Type Type;
    union
    {
        cim_window_state Window;
    } For;
} cim_ui_state;

typedef struct cim_state_node
{
    cim_ui_state           State;
    struct cim_state_node *Parent;
    struct sim_state_node *Child;
} cim_state_node;

typedef struct cim_point_node
{
    cim_point              Value;
    struct cim_point_node *Prev;
    struct cim_point_node *Next;
} cim_point_node;

typedef struct cim_state_entry
{
    char            Key[64];
    cim_state_node *Value;
    cim_u32         Hashed;
} cim_state_entry;

typedef struct cim_state_hashmap
{
    cim_u8          *Metadata;
    cim_state_entry *Buckets;
    cim_u32          GroupCount;
    bool             IsInitialized;
} cim_state_hashmap;

typedef struct cim_primitive_rings
{
    cim_state_node    StateNodes[1024];
    cim_u32           AllocatedStateNodes;
    cim_u32           StateNodesCapacity;
    cim_state_hashmap StateMap;

    cim_point_node PointNodes[1024];
    cim_u32        PointCount;
    cim_u32        PointNodesCapacity;
} cim_primitive_rings;

#define CimEmptyBucketTag 0x80
#define CimBucketGroupSize 16

void CimMap_AddStateEntry(const char *Key, cim_state_node *Value, cim_state_hashmap *Hashmap);
cim_state_node *CimMap_GetStateValue(const char *Key, cim_state_hashmap *Hashmap);
cim_state_node *CimRing_AddStateNode(cim_ui_state State, cim_primitive_rings *Rings);
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

void CimConstraint_SolveAll();
void CimConstraint_Register(CimConstraint_Type Type, void *Context);

// } [SECTION:Constraints]

// [SECTION:Commands] {

typedef enum CimTopo_Type
{
    CimTopo_Quad,
} CimTopo_Type;

typedef struct cim_quad
{
    cim_point p0, p1, p2, p3;
} cim_quad;

typedef struct cim_topo_quad
{
    cim_point_node *Start;
} cim_topo_quad;

typedef struct cim_command
{
    CimTopo_Type Type;
    union
    {
        cim_topo_quad Quad;
    } For;

    struct cim_command *Prev;
    struct cim_command *Next;
} cim_command;

typedef struct cim_command_ring
{
    cim_command Pool[1024];
    cim_u32     Count;
    cim_u32     Capacity;

    cim_command *Rings[4];
    cim_u32      RingCount;
    cim_u32      RingCapacity;

    cim_command *CurrentHead;
    cim_command *CurrentTail;
} cim_command_ring;

void
CimCommand_StartCommandRing(cim_command_ring *Commands);

cim_command *
CimCommand_PushQuad  (cim_point_node *QuadStart);

cim_command *
CimCommand_AppendQuad(cim_point_node *QuadStart, cim_command *To);

// } [SECTION:Commands]

// } [SECTION:Widgets]

typedef struct cim_context
{
    void *Backend;

    cim_primitive_rings PrimitiveRings;

    cim_command_ring Commands;

    cim_constraint_manager ConstraintManager;

    cim_io_inputs Inputs;
} cim_context;

extern cim_context *CimContext;

#ifdef __cplusplus
}
#endif
