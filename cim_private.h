#pragma once

#include "cim.h"

typedef struct
{
   cim_u32 Stride;
   void   *Data;
} cim_vertex;

typedef struct
{
   cim_u32 VtxOffset;
   cim_u32 IdxOffset;
   cim_u32 IdxCount;
} cim_draw_command;

typedef struct
{
    cim_vertex Vtxs;
    cim_u32   *Idxs;

    cim_draw_command *Commands;
    cim_u32           CommandCount;
} cim_draw_list;

typedef enum
{
    CimUINode_Node,

    CimUINode_Window,
    CimUINode_Button,
    CimUINode_Text,
} CimUINode_Type;

typedef struct
{
    bool SomeBoolean;
} cim_window_state;

typedef struct
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

typedef struct
{
    void   *PushBase;
    cim_u32 PushSize;
    cim_u32 PushCapacity;

    cim_ui_node  RootNode;
    cim_ui_node *NextNode;

    bool IsInitialized;
} cim_ui_tree;

typedef struct
{
    cim_draw_list *Lists;
    cim_u32        ListCount;

    cim_ui_tree Tree;
} cim_context;

extern cim_context *CimCtx;

cim_ui_node *CreateNode(cim_ui_tree *Tree, const char *Data);
void AddChild(cim_ui_node *Parent, cim_ui_node *Child);
void PrintNode(cim_ui_node *Node, sml_u32 Depth);
