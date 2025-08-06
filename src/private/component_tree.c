#include "cim_private.h"

#include <string.h> // memset

// NOTE: I now have a better idea of what to do:
// 1) Create the structure which holds all of the logic together:
//      The hashmap, the trees, the state tracking.
// 2) Slightly rework the hashmap such that it updates the state correctly/stores
//      the correct information
// 3) Write a stupid that somewhat does its job.
// 4) Profit.

typedef struct ctree_node
{
    struct
    ctree_node **Children;
    cim_u32      ChildCount;
    cim_u32      ChildSize;
    component    Component;
} ctree_node;

typedef struct component_entry 
{
    char        Key[64];
    ctree_node *Node;    // WARN: This might be dangerous.
} component_entry;

typedef struct component_hashmap 
{
    cim_u8          *Metadata;
    component_entry *Buckets;
    cim_u32          GroupCount;
    bool             IsInitialized;
} component_hashmap;

typedef struct ctree
{
    component_hashmap ComponentMap; // Maps ID -> Node

    ctree_node  Root;      // The single root (Simplified to one for now)
    ctree_node *Nodes;     // A pool of nodes used by all the trees.
    cim_u32     NodeCount; // Nodes allocated count
    cim_u32     NodeSize;  // Nodes allocated size

    ctree_node *AtNode; // The state, where we are.
} ctree;

static inline ctree_node *
AllocateNode(ctree *Tree)
{
    if(!Tree)
    {
        CimLog_Fatal("Internal CTree: Tree does not exist.");
        return NULL;
    }

    if(Tree->NodeCount == Tree->NodeSize)
    {
        Tree->NodeSize = Tree->NodeSize ? Tree->NodeSize * 2 : 8;

        ctree_node *New = malloc(Tree->NodeSize * sizeof(ctree_node));
        if(!New)
        {
            CimLog_Fatal("Internal CTree: Malloc failure (OOM?)");
            return NULL;
        }

        if(Tree->Nodes)
        {
            memcpy(New, Tree->Nodes, Tree->NodeCount * sizeof(ctree_node));
            free(Tree->Nodes);
        }

        Tree->Nodes = New;
    }

    ctree_node *Node = Tree->Nodes + Tree->NodeCount++;
    return Node;
}

#ifdef __cplusplus
extern "C" {
#endif

component *
CimComponent_GetOrInsert(const char *Key,
                         bool        IsRoot) 
{
    cim_context *Ctx = CimContext; Cim_Assert(Ctx);
    if (!Ctx->CTree)
    {
        Ctx->CTree = malloc(sizeof(ctree)); Cim_Assert(Ctx->CTree);
        if (!Ctx->CTree)
        {
            CimLog_Fatal("Internal CTree: Cannot heap-allocate.");
            return NULL;
        }

        memset(Ctx->CTree, 0, sizeof(ctree));
    }

    ctree             *CTree   = Ctx->CTree;
    component_hashmap *Hashmap = &CTree->ComponentMap;

    if (!Hashmap->IsInitialized)
    {
        Hashmap->GroupCount = 32;

        cim_u32 BucketCount  = Hashmap->GroupCount * CimBucketGroupSize;
        size_t  BucketSize   = BucketCount * sizeof(component_entry);
        size_t  MetadataSize = BucketCount * sizeof(cim_u8);

        Hashmap->Buckets  = malloc(BucketSize);
        Hashmap->Metadata = malloc(MetadataSize);

        if (!Hashmap->Buckets || !Hashmap->Metadata)
        {
            return NULL;
        }

        memset(Hashmap->Buckets , 0                , BucketSize);
        memset(Hashmap->Metadata, CimEmptyBucketTag, MetadataSize);

        Hashmap->IsInitialized = true;
    }

    cim_u32 ProbeCount = 0;
    cim_u32 Hashed     = CimHash_String32(Key);
    cim_u32 GroupIdx   = Hashed & (Hashmap->GroupCount - 1);

    while (true)
    {
        cim_u8 *Meta = Hashmap->Metadata + GroupIdx * CimBucketGroupSize;
        cim_u8  Tag  = (Hashed & 0x7F);

        __m128i Mv = _mm_loadu_si128((__m128i*)Meta);
        __m128i Tv = _mm_set1_epi8(Tag);

        cim_i32 TagMask = _mm_movemask_epi8(_mm_cmpeq_epi8(Mv, Tv));
        while (TagMask)
        {
            cim_u32 Lane = CimHash_FindFirstBit32(TagMask);
            cim_u32 Idx  = (GroupIdx * CimBucketGroupSize) + Lane;

            // NOTE: A bit goofy?
            component_entry *E = Hashmap->Buckets + Idx;
            if (strcmp(E->Key, Key) == 0)
            {
                CTree->AtNode = E->Node;
                return &E->Node->Component;
            }

            TagMask &= TagMask - 1;
        }

        __m128i Ev        = _mm_set1_epi8(CimEmptyBucketTag);
        cim_i32 EmptyMask = _mm_movemask_epi8(_mm_cmpeq_epi8(Mv, Ev));
        if (EmptyMask)
        {
            cim_u32 Lane = CimHash_FindFirstBit32(EmptyMask);
            cim_u32 Idx  = (GroupIdx * CimBucketGroupSize) + Lane;

            Meta[Lane] = Tag;

            component_entry *E = Hashmap->Buckets + Idx;
            strcpy_s(E->Key, sizeof(E->Key), Key);
            E->Key[sizeof(E->Key)-1] = 0;

            if(IsRoot)
            {
                E->Node = &CTree->Root;
                return &CTree->Root.Component; // We only have one root.
            }
            else
            {
                // WARN: We probably don't want to do this (Alloc). But it is okay for a prototype.

                ctree_node *Node = CTree->AtNode;
                if (Node->ChildCount == Node->ChildSize)
                {
                    Node->ChildSize = Node->ChildSize ? Node->ChildSize * 2 : 4;

                    ctree_node **New = malloc(Node->ChildSize * sizeof(ctree_node*));
                    if (!New)
                    {
                        CimLog_Fatal("Internal CTree: Malloc failure (OOM?)");
                        return NULL;
                    }

                    if (Node->Children)
                    {
                        memcpy(New, Node->Children, Node->ChildCount * sizeof(ctree_node));
                        free(Node->Children);
                    }

                    Node->Children = New;
                }

                E->Node = AllocateNode(CTree);
                Node->Children[Node->ChildCount] = E->Node;
            }

            CTree->AtNode = E->Node;

            return &E->Node->Component;
        }

        ProbeCount++;
        GroupIdx = (GroupIdx + ProbeCount * ProbeCount) & (Hashmap->GroupCount - 1);
    }
}

#ifdef __cplusplus
}
#endif
