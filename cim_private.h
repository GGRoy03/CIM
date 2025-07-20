#pragma once

#include "cim.h"

typedef enum CimUINode_Type
{
    CimUINode_Node,

    CimUINode_Window,
    CimUINode_Button,
    CimUINode_Text,
} CimUINode_Type;

typedef struct cim_window_state
{
    bool SomeBoolean;
} cim_window_state;

typedef struct cim_ui_node
{
    char Name[64];

    struct cim_ui_node **Children;

    cim_u32 ChildCount;
    cim_u32 ChildCapacity;

    CimUINode_Type Type;
    union
    {
        cim_window_state Window;
    } State;

} cim_ui_node;

typedef struct cim_ui_tree
{
    void   *PushBase;
    cim_u32 PushSize;
    cim_u32 PushCapacity;

    cim_ui_node  RootNode;
    cim_ui_node *NextNode;

    bool IsInitialized;
} cim_ui_tree;

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ============================================================
// PRIVATE TYPE DEFINITIONS FOR  CIM. BY SECTION.
// -[SECTION] Hashing
// -[SECTION] Primitives
// -[SECTION] Holders
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
    sml_f32 x, y;
} cim_point;

// } [SECTION:Primitives]

// [SECTION:Holders] {

typedef enum CimHolder_Type
{
    CimHolder_Fixed,
    CimHolder_Moving,
} CimHolder_Type;

typedef struct cim_holder
{
    CimHolder_Type        Type;
    cim_primitive_points *Points;
    cim_u32               PointCount;
};

// } [SECTION:Holders]

// [SECTION:Constraints] {

typedef void cim_constraint_function (void *Context);

static void cim_constraint_drag(void *Context);

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
    // Holder
    cim_holder Holder;

    // Set of constraints
    cim_constraint *Constraints[4];
    cim_u32         ConstraintCount;

    // Set of topos to draw
    cim_topo Topos[4];
    cim_u32  TopoCount;

    // Extra data
    cim_f32       c0, c1, c2, c3;
    cim_bit_field Flags;
    bool          Opened;
} cim_window;

// } [SECTION:Widgets]

typedef struct cim_context
{
    // Push buffer
    char   *PushBase;
    cim_u32 PushSize;
    cim_u32 PushCapacity;

    // Backend opaque handle.
    void *Backend;

    // IO state
    cim_io_inputs Inputs;
} cim_context;

extern cim_context *CimContext;

#ifdef __cplusplus
}
#endif
