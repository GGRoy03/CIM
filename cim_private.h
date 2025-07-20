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
// -[SECTION] Commands
// ============================================================
// ============================================================

// -[SECTION] Hashing {

#define FNV32Prime 0x01000193
#define FNV32Basis 0x811C9DC5

cim_u32 Cim_FindFirstBit32(cim_u32 Mask);
cim_u32 Cim_HashString(const char* String);

// } -[SECTION:Hashing]

// -[SECTION:Commands] {

typedef enum CimRenderCommand_Type
{
    CimRenderCommand_None,

    CimRenderCommand_CreateMaterial,
    CimRenderCommand_DestroyMaterial,
} CimRenderCommand_Type;

typedef void cim_renderer_playback(void *PushBuffer, size_t PushBufferSize);

typedef struct cim_render_command_header
{
    CimRenderCommand_Type Type;
    cim_u32               Size;
} cim_render_command_header;

typedef struct cim_payload_create_material
{
    char          UserID[64];
    cim_bit_field Features;
} cim_payload_create_material;

typedef struct cim_payload_destroy_material
{
    char UserID[64];
} cim_payload_destroy_material;

void CimInt_PushRenderCommand(cim_render_command_header *Header, void *Payload);

// } -[SECTION:Commands]

cim_ui_node *CreateNode(cim_ui_tree *Tree, const char *Data);
void AddChild(cim_ui_node *Parent, cim_ui_node *Child);
void PrintNode(cim_ui_node *Node, cim_u32 Depth);

typedef struct cim_context
{
    // Push buffer
    char   *PushBase;
    cim_u32 PushSize;
    cim_u32 PushCapacity;

    // Backend opaque handle.
    void *Backend;
} cim_context;

extern cim_context *CimContext;

#ifdef __cplusplus
}
#endif
