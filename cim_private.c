#include "cim.h"
#include "cim_private.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

cim_context *CimCtx;

void InitializeContext()
{
    CimCtx = malloc(sizeof(cim_context));
    memset(CimCtx, 0, sizeof(cim_context));
}
