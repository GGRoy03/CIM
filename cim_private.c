#include "cim.h"
#include "cim_private.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef __cplusplus
extern "C" {
#endif

void* AllocateInTree(cim_ui_tree *Tree, size_t AllocSize)
{
    if(!Tree->IsInitialized)
    {
        Tree->PushBase     = malloc(1024);
        Tree->PushSize     = 0;
        Tree->PushCapacity = 1024;

        Tree->IsInitialized = true;
    }

    if(Tree->PushSize + AllocSize > Tree->PushCapacity)
    {
        return NULL;
    }

    void *Block     = (char*)Tree->PushBase + Tree->PushSize;
    Tree->PushSize += (cim_u32)AllocSize;

    return Block;
}

cim_ui_node* CreateNode(cim_ui_tree *Tree, const char *Name)
{
    cim_ui_node *Node = AllocateInTree(Tree, sizeof(cim_ui_node));
    Node->Children      = AllocateInTree(Tree, sizeof(cim_ui_node*) * 4);
    Node->ChildCount    = 0;
    Node->ChildCapacity = 4;
    strcpy_s(Node->Name, sizeof(Node->Name), Name);

    return Node;
}

void AddChild(cim_ui_node *Parent, cim_ui_node *Child)
{
    if(Parent->ChildCount >= Parent->ChildCapacity)
    {
        return;
    }

    Parent->Children[Parent->ChildCount++] = Child;
}

void PrintNode(cim_ui_node *Node, cim_u32 Depth)
{
    for(cim_u32 Indent = 0; Indent < Depth; Indent++)
    {
        printf("  ");
    }
    printf("%s\n", Node->Name);

    for(cim_u32 Child = 0; Child < Node->ChildCount; Child++)
    {
        PrintNode(Node->Children[Child], Depth + 1);
    }
}

// ============================================================
// ============================================================
// PRIVATE IMPLEMENTATION FOR CIM. BY SECTION.
// -[SECTION] Hashing
// -[SECTION] Commands
// ============================================================
// ============================================================

// -[SECTION] Hashing {

cim_u32 Cim_FindFirstBit32(cim_u32 Mask)
{
    unsigned long Index = 0;
    _BitScanForward(&Index, Mask);
    return (cim_u32)Index;
}

cim_u32 Cim_HashString(const char* String)
{
    if (!String)
    {
        Cim_Assert(!"Cannot hash empty string.");
        return 0;
    }

    cim_u32 Result = FNV32Basis;
    cim_u8  Character;

    while ((Character = *String++))
    {
        Result = FNV32Prime * (Result ^ Character);
    }

    return Result;
}

// } -[SECTION:Hashing]

// -[SECTION:Commands] {

void CimInt_PushRenderCommand(cim_render_command_header *Header, void *Payload)
{
    cim_context *Ctx = CimContext;

    size_t SizeNeeded = sizeof(cim_render_command_header) + Header->Size;
    if (Ctx->PushSize + SizeNeeded <= Ctx->PushCapacity)
    {
        memcpy(Ctx->PushBase + Ctx->PushSize, Header, sizeof(cim_render_command_header));
        Ctx->PushSize += sizeof(cim_render_command_header);

        memcpy(Ctx->PushBase + Ctx->PushSize, Payload, Header->Size);
        Ctx->PushSize += Header->Size;
    }
    else
    {
        // NOTE: Do we resize, maybe user chooses? Doesn't matter for now.
        // Just abort.
        abort();
    }
}

// } -[SECTION:Commands]

cim_context *CimContext;

#ifdef __cplusplus
}
#endif