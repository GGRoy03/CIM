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
    cim_f32 x, y;
} cim_point;

// } [SECTION:Primitives]

// [SECTION:Holders] {

typedef enum CimHolder_Type
{
    CimHolder_Fixed,
    CimHolder_Moving,
} CimHolder_Type;

// WARN: For now a holder is forced to 4 points (It should hold pointers/idcs?)
typedef struct cim_holder
{
    CimHolder_Type Type;
    cim_point      Points[4];
    cim_u32        PointCount;
} cim_holder;

// } [SECTION:Holders]

// [SECTION:Constraints] {

typedef enum CimConstraint_Type
{
    CimConstraint_Drag,

    CimConstraint_Count,
} CimConstraint_Type;

typedef struct cim_context_drag
{
    cim_holder *Holder;
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
    // Holder
    cim_holder Holder;

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

    // Our new constraint manager
    cim_constraint_manager ConstraintManager;

    // IO state
    cim_io_inputs Inputs;
} cim_context;

extern cim_context *CimContext;

#ifdef __cplusplus
}
#endif
