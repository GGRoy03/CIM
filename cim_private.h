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
// -[SECTION] Topos
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
} cim_primitive_rings;

#define CimEmptyBucketTag 0x80
#define CimBucketGroupSize 16

void CimMap_AddStateEntry(const char *Key, cim_state_node *Value, cim_state_hashmap *Hashmap);
cim_state_node *CimMap_GetStateValue(const char *Key, cim_state_hashmap *Hashmap);
cim_state_node *CimRing_AddStateNode(cim_ui_state State, cim_primitive_rings *Rings);

// } [SECTION:Primitives]

// [SECTION:Constraints] {

typedef enum CimConstraint_Type
{
    CimConstraint_Drag,

    CimConstraint_Count,
} CimConstraint_Type;

typedef struct cim_context_drag
{
    cim_point *Points[4];
} cim_context_drag;

typedef struct cim_constraint_manager
{
    cim_context_drag DragCtxs[4];
    cim_u32          RegDragCtxs;

} cim_constraint_manager;

void CimConstraint_SolveAll();
void CimConstraint_Register(CimConstraint_Type Type, void *Context);

// } [SECTION:Constraints]

// [SECTION:Topos] {

typedef enum CimTopo_Type
{
    CimTopo_Quad,
    CimTopo_Borders,
    CimTopo_Polyline,
} CimTopo_Type;

typedef struct cim_topo
{
    CimTopo_Type Type;
    void        *Data;
} cim_topo;

// } [SECTION:Topos]

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

    cim_constraint_manager ConstraintManager;

    cim_io_inputs Inputs;
} cim_context;

extern cim_context *CimContext;

#ifdef __cplusplus
}
#endif
