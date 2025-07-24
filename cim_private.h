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

typedef struct cim_point_node
{
    cim_point              Value;
    struct cim_point_node *Prev;
    struct cim_point_node *Next;
} cim_point_node;

typedef enum CimUIState_Type
{
    CimUIState_Window,
} CimUIState_Type;

typedef struct cim_window_state
{
    cim_point_node *HeadFirstPoint;
    cim_point_node *BodyFirstPoint;
    bool            Closed;
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

typedef enum CimCommand_Type
{
    CimCommand_Quad,
} CimCommand_Type;

typedef struct cim_command_buffer
{
    char  *Base;
    size_t Size;
    size_t Capacity;
} cim_command_buffer;

typedef struct cim_command_header
{
    CimCommand_Type Type;
    cim_u32         Size;
} cim_command_header;

typedef struct cim_payload_quad
{
    cim_point_node *Point;
    cim_vector4     Color;
} cim_payload_quad;

void CimCommand_PushQuad(cim_point_node *FirstPoint, cim_vector4 Color);
void CimCommand_Push(cim_command_header *Header, void *Payload);

// } [SECTION:Commands]

// [SECTION:Widgets] {

typedef struct cim_window
{
    // Primitives

    // State
    bool Opened;
} cim_window;

// } [SECTION:Widgets]

typedef struct cim_context
{
    void *Backend;

    cim_primitive_rings PrimitiveRings;

    cim_command_buffer Commands;

    cim_constraint_manager ConstraintManager;

    cim_io_inputs Inputs;
} cim_context;

extern cim_context *CimContext;

#ifdef __cplusplus
}
#endif
