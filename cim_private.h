#pragma once

#include "cim.h"

typedef struct cim_draw_command
{
   cim_u32 VtxOffset;
   cim_u32 IdxOffset;
   cim_u32 IdxCount;
} cim_draw_command;

typedef struct cim_draw_list
{
    void    *Vtxs;
    cim_u32 *Idxs;

    cim_draw_command *Commands;
    cim_u32           CommandCount;
} cim_draw_list;

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

typedef struct cim_context
{
    cim_draw_list *Lists;
    cim_u32        ListCount;

    cim_ui_tree Tree;
} cim_context;

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ============================================================
// PRIVATE TYPE DEFINITIONS FOR  CIM. BY SECTION.
// -[SECTION] Hashing
// ============================================================
// ============================================================

// -[SECTION] Hashing {

#define FNV32Prime 0x01000193
#define FNV32Basis 0x811C9DC5

cim_u32 Cim_FindFirstBit32(cim_u32 Mask);
cim_u32 Cim_HashString(const char* String);

// } -[SECTION:Hashing]

cim_ui_node *CreateNode(cim_ui_tree *Tree, const char *Data);
void AddChild(cim_ui_node *Parent, cim_ui_node *Child);
void PrintNode(cim_ui_node *Node, cim_u32 Depth);

#ifdef __cplusplus
}
#endif
