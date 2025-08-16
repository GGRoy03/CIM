#pragma once

// [Headers] =============================

// TODO: Remove dependencies on this.
#include <stdint.h>
#include <stdbool.h> 
#include <string.h>  // memcpy, memset, ...
#include <stdlib.h>  // malloc, free, ...
#include <intrin.h>  // SIMD
#include <float.h>   // Numeric Limits
#include <math.h>    // Min/Max
#include <stdio.h>   // Reading files 

typedef uint8_t  cim_u8;
typedef uint16_t cim_u16;
typedef uint32_t cim_u32;
typedef int      cim_i32;
typedef float    cim_f32;
typedef double   cim_f64;
typedef cim_u32  cim_bit_field;

#define Cim_Unused(Arg) (void*)Arg
#define Cim_Assert(Cond) do { if (!(Cond)) __debugbreak(); } while (0)
#define CIM_ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))
#define Cim_IsPowerOfTwo(Value) (((Value) & ((Value) - 1)) == 0)
#define Cim_SafeRatio(A, B) ((B) ? ((A)/(B)) : (A))

// [Private API] =============================

typedef struct cim_vector2
{
    cim_f32 x, y;
} cim_vector2;

typedef struct cim_vector4
{
    cim_f32 x, y, z, w;
} cim_vector4;

typedef struct buffer
{
    cim_u8 *Data;
    cim_u32 At;
    cim_u32 Size;
} buffer;

typedef struct cim_rect
{
    cim_u32 MinX;
    cim_u32 MinY;
    cim_u32 MaxX;
    cim_u32 MaxY;
} cim_rect;

// [Logging:Types]

typedef enum CimLog_Severity
{
    CimLog_Info    = 0,
    CimLog_Warning = 1,
    CimLog_Error   = 2,
    CimLog_Fatal   = 3,
} CimLog_Severity;

typedef void CimLogFn(CimLog_Severity Severity, const char *File, cim_i32 Line, const char *Format, va_list Args);
CimLogFn *CimLogger;

#define CimLog_Info(...)  Cim_Assert(CimLogger); Cim_Log(CimLog_Info   , __FILE__, __LINE__, __VA_ARGS__);
#define CimLog_Warn(...)  Cim_Assert(CimLogger); Cim_Log(CimLog_Warning, __FILE__, __LINE__, __VA_ARGS__)
#define CimLog_Error(...) Cim_Assert(CimLogger); Cim_Log(CimLog_Error  , __FILE__, __LINE__, __VA_ARGS__)
#define CimLog_Fatal(...) Cim_Assert(CimLogger); Cim_Log(CimLog_Fatal  , __FILE__, __LINE__, __VA_ARGS__)

void Cim_Log(CimLog_Severity Level, const char *File, cim_i32 Line, const char *Format, ...);

// [Arena:Types]
// NOTE: Probably shouldn't exist.

typedef struct cim_arena
{
    void *Memory;
    size_t At;
    size_t Capacity;
} cim_arena;

// [IO:Types]

#define CimIO_MaxPath 256
#define CimIO_KeyboardKeyCount 256
#define CimIO_KeybordEventByufferSize 128

typedef enum CimMouse_Button
{
    CimMouse_Left        = 0,
    CimMouse_Right       = 1,
    CimMouse_Middle      = 2,
    CimMouse_ButtonCount = 3,
} CimMouse_Button;

typedef struct cim_button_state
{
    bool    EndedDown;
    cim_u32 HalfTransitionCount;
} cim_button_state;

typedef struct cim_keyboard_event
{
    cim_u8 VKCode;
    bool   IsDown;
} cim_keyboard_event;

typedef struct cim_inputs
{
    cim_button_state Buttons[CimIO_KeyboardKeyCount];
    cim_f32          ScrollDelta;
    cim_i32          MouseX, MouseY;
    cim_i32          MouseDeltaX, MouseDeltaY;
    cim_button_state MouseButtons[5];
} cim_inputs;

typedef struct cim_watched_file
{
    char FileName[CimIO_MaxPath];
    char FullPath[CimIO_MaxPath];
} cim_watched_file;

typedef struct cim_file_watcher_context
{
    char              Directory[CimIO_MaxPath];
    cim_watched_file *Files;
    cim_u32           FileCount;
} cim_file_watcher_context;

// [Layout:Types]

#define CimLayout_MaxNestDepth 32
#define CimLayout_InvalidNode 0xFFFFFFFF
#define CimLayout_MaxShapesForDrag 4
#define CimLayout_MaxDragPerBatch 16

typedef enum CimComponent_Flag
{
    CimComponent_Invalid = 0,
    CimComponent_Window  = 1 << 0,
    CimComponent_Button  = 1 << 1,
} CimComponent_Flag;

typedef enum Layout_Order
{
    Layout_Horizontal = 0,
    Layout_Vertical   = 1,
} Layout_Order;

typedef struct cim_window_style
{
    // Style
    cim_vector4 Color;
    cim_u32     BorderWidth;
    cim_vector4 BorderColor;

    // Layout
    cim_vector2  Size;
    cim_vector2  Spacing;
    cim_vector4  Padding;
    Layout_Order Order;
} cim_window_style;

typedef struct cim_window
{
    cim_window_style Style;

    cim_i32 LastFrameScreenX;
    cim_i32 LastFrameScreenY;

    bool IsInitialized;
} cim_window;

typedef struct cim_button_style
{
    cim_vector4 Color;
    cim_u32     BorderWidth;
    cim_vector4 BorderColor;

    cim_vector2 Size;
} cim_button_style;

typedef struct cim_button
{
    cim_button_style Style;
} cim_button;

typedef struct cim_component
{
    bool IsInitialized;

    cim_u32 LayoutNodeIndex;

    CimComponent_Flag Type;
    union
    {
        cim_window Window;
        cim_button Button;
    } For;
} cim_component;

typedef struct cim_component_entry
{
    cim_component Value;
    char          Key[64];
} cim_component_entry;

typedef struct cim_component_hashmap
{
    cim_u8              *Metadata;
    cim_component_entry *Buckets;
    cim_u32              GroupCount;
    bool                 IsInitialized;
} cim_component_hashmap;

typedef struct cim_layout_node
{
    // Hierarchy
    cim_u32 Id;
    cim_u32 Parent;
    cim_u32 FirstChild;
    cim_u32 LastChild;
    cim_u32 NextSibling;

    // Layout Intent
    cim_f32 ContentWidth;
    cim_f32 ContentHeight;
    cim_f32 PrefWidth;
    cim_f32 PrefHeight;

    // Arrange Result
    cim_f32 X;
    cim_f32 Y;
    cim_f32 Width;
    cim_f32 Height;

    // Layout..
    cim_vector2  Spacing;
    cim_vector4  Padding;
    Layout_Order Order;

    // State
    bool Clicked;
    bool Held;
} cim_layout_node;

typedef struct cim_layout_tree
{
    // Memory pool
    cim_layout_node *Nodes;
    cim_u32          NodeCount;
    cim_u32          NodeSize;

    // Tree-Logic
    cim_u32 ParentStack[CimLayout_MaxNestDepth];
    cim_u32 AtParent;

    cim_u32 FirstDragNode;  // Set transforms to 0 when we pop that node.
    cim_i32 DragTransformX;
    cim_i32 DragTransformY;
} cim_layout_tree;

typedef struct cim_layout
{
    cim_layout_tree       Tree; // Forced to 1 tree for now.
    cim_component_hashmap Components;
} cim_layout;

// [Styling:Types]

#define FAIL_ON_NEGATIVE(Negative, Message, ...) if(Negative) {CimLog_Error(Message, __VA_ARGS__);}
#define ARRAY_TO_VECTOR2(Array, Length, Vector)  if(Length > 0) Vector.x = Array[0]; if(Length > 1) Vector.y = Array[1];
#define ARRAY_TO_VECTOR4(Array, Length, Vector)  if(Length > 0) Vector.x = Array[0]; if(Length > 1) Vector.y = Array[1]; \
                                                 if(Length > 2) Vector.z = Array[2]; if(Length > 3) Vector.w = Array[3];

typedef enum Attribute_Type
{
    Attribute_Size        = 0,
    Attribute_Color       = 1,
    Attribute_Padding     = 2,
    Attribute_Spacing     = 3,
    Attribute_LayoutOrder = 4,
    Attribute_BorderColor = 5,
    Attribute_BorderWidth = 6,
} Attribute_Type;

typedef enum Token_Type
{
    Token_String     = 255,
    Token_Identifier = 256,
    Token_Number     = 257,
    Token_Assignment = 258,
    Token_Vector     = 269,
} Token_Type;

typedef struct master_style
{
    // Styling
    cim_vector4 Color;
    cim_vector4 BorderColor;
    cim_u32     BorderWidth;

    // Layout/Positioning
    cim_vector2  Size;
    cim_vector2  Spacing;
    cim_vector4  Padding;
    Layout_Order Order;
} master_style;

typedef struct style_desc
{
    char              Id[64];
    CimComponent_Flag ComponentFlag;
    master_style      Style;
} style_desc;

typedef struct token
{
    Token_Type Type;

    union
    {
        cim_f32 Float32;
        cim_u32 UInt32;
        cim_i32 Int32;
        struct { cim_u8 *Name; cim_u32 NameLength; };
        struct { cim_f32 Vector[4]; cim_u32 VectorSize; };
    };
} token;

typedef struct lexer
{
    token *Tokens;
    cim_u32 TokenCount;
    cim_u32 TokenCapacity;

    bool IsValid;
} lexer;

typedef struct user_styles
{
    style_desc *Descs;
    cim_u32     DescCount;
    cim_u32     DescSize;

    bool IsValid;
} user_styles;

typedef struct valid_component
{
    const char       *Name;
    size_t            Length;
    CimComponent_Flag ComponentFlag;
} valid_component;

typedef struct valid_attribute
{
    const char    *Name;
    size_t         Length;
    Attribute_Type Type;
    cim_bit_field  ComponentFlag;
} valid_attribute;

// [Render Commands]

typedef struct cim_draw_command
{
    size_t  VtxOffset;
    size_t  IdxOffset;
    cim_u32 IdxCount;
    cim_u32 VtxCount;

    cim_rect      ClippingRect;
    cim_bit_field Features;
} cim_draw_command;

typedef struct cim_command_buffer
{
    cim_arena FrameVtx;
    cim_arena FrameIdx;

    cim_draw_command *Commands;
    cim_u32           CommandCount;
    cim_u32           CommandSize;

    bool ClippingRectChanged;
    bool FeatureStateChanged;
} cim_command_buffer;

// [Global Context]

typedef enum CimContext_State
{
    CimContext_Invalid     = 0,
    CimContext_Layout      = 1,
    CimContext_Interaction = 2,
} CimContext_State;

typedef struct cim_context
{
    CimContext_State   State;
    cim_layout         Layout;
    cim_inputs         Inputs;
    cim_command_buffer Commands;
    void              *Backend;
} cim_context;

static cim_context *CimCurrent;

#define UI_STATE      (CimCurrent->State)
#define UI_LAYOUT     (CimCurrent->Layout)
#define UIP_LAYOUT   &(CimCurrent->Layout)
#define UI_INPUT      (CimCurrent->Inputs)
#define UIP_INPUT    &(CimCurrent->Inputs)
#define UI_COMMANDS   (CimCurrent->Commands)
#define UIP_COMMANDS &(CimCurrent->Commands)

void Cim_InitContext(cim_context *UserContext)
{
    if (!UserContext)
    {
        CimLog_Fatal("Argument Exception(1): Value is NULL.");
    }

    memset(UserContext, 0, sizeof(cim_context));
    CimCurrent = UserContext;
}

// [Private API Implementation] =============================

// [Logging]

void
Cim_Log(CimLog_Severity Level, const char *File, cim_i32 Line, const char *Format, ...)
{
    if (!CimLogger)
    {
        return;
    }
    else
    {
        va_list Args;
        __crt_va_start(Args, Format);
        __va_start(&Args, Format);
        CimLogger(Level, File, Line, Format, Args);
        __crt_va_end(Args);
    }
}

// [Hashing]

#define FNV32Prime 0x01000193
#define FNV32Basis 0x811C9DC5
#define CimEmptyBucketTag  0x80
#define CimBucketGroupSize 16

cim_u32
CimHash_FindFirstBit32(cim_u32 Mask)
{
    return (cim_u32)__builtin_ctz(Mask);
}

cim_u32
CimHash_String32(const char *String)
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

cim_u32
CimHash_Block32(void *ToHash, cim_u32 ToHashLength)
{
    cim_u8 *BytePointer = (cim_u8 *)ToHash;
    cim_u32 Result = FNV32Basis;

    if (!ToHash)
    {
        CimLog_Error("Called Hash32Bits with NULL memory adress as value.");
        return 0;
    }

    if (ToHashLength < 1)
    {
        CimLog_Error("Called Hash32Bits with length inferior to 1.");
        return 0;
    }

    while (ToHashLength--)
    {
        Result = FNV32Prime * (Result ^ *BytePointer++);
    }

    return Result;
}

// [IO]

bool
IsMouseDown(CimMouse_Button MouseButton, cim_inputs *Inputs)
{
    bool IsDown = Inputs->MouseButtons[MouseButton].EndedDown;

    return IsDown;
}

bool
IsMouseReleased(CimMouse_Button MouseButton, cim_inputs *Inputs)
{
    cim_button_state *State = &Inputs->MouseButtons[MouseButton];
    bool IsReleased = (!State->EndedDown) && (State->HalfTransitionCount > 0);

    return IsReleased;
}

bool
IsMouseClicked(CimMouse_Button MouseButton, cim_inputs *Inputs)
{
    cim_button_state *State = &Inputs->MouseButtons[MouseButton];
    bool IsClicked = (State->EndedDown) && (State->HalfTransitionCount > 0);

    return IsClicked;
}

cim_i32
GetMouseDeltaX(cim_inputs *Inputs)
{
    cim_i32 DeltaX = Inputs->MouseDeltaX;

    return DeltaX;
}

cim_i32
GetMouseDeltaY(cim_inputs *Inputs)
{
    cim_i32 DeltaY = Inputs->MouseDeltaY;

    return DeltaY;
}

cim_vector2
GetMousePosition(cim_inputs *Inputs)
{
    cim_vector2 Position = (cim_vector2){ (cim_f32)Inputs->MouseX, (cim_f32)Inputs->MouseY };

    return Position;
}

// [Geometry]

bool
IsInsideRect(cim_rect Rect)
{
    cim_vector2 MousePos = GetMousePosition(UIP_INPUT);

    bool MouseIsInside = (MousePos.x > Rect.MinX) && (MousePos.x < Rect.MaxX) &&
                         (MousePos.y > Rect.MinY) && (MousePos.y < Rect.MaxY);

    return MouseIsInside;
}

cim_rect
MakeRectFromNode(cim_layout_node *Node)
{
    cim_rect Rect;
    Rect.MinX = Node->X;    
    Rect.MinY = Node->Y;
    Rect.MaxX = Node->X + Node->Width;
    Rect.MaxY = Node->Y + Node->Height;

    return Rect;
}

// [Arenas]
// NOTE:
// 1) Should probably not exist.

void 
WriteToArena(void *Data, size_t Size, cim_arena *Arena)
{
    if (Arena->At + Size > Arena->Capacity)
    {
        Arena->Capacity = Arena->Capacity ? Arena->Capacity * 2 : 1024;

        while (Arena->Capacity < Arena->At + Size)
        {
            Arena->Capacity += (Arena->Capacity >> 1);
        }

        void *New = malloc(Arena->Capacity);
        if (!New)
        {
            Cim_Assert(!"Malloc failure: OOM?");
            return;
        }
        else
        {
            memset((char *)New + Arena->At, 0, Arena->Capacity - Arena->At);
        }

        if (Arena->Memory)
        {
            memcpy(New, Arena->Memory, Arena->At);
            free(Arena->Memory);
        }

        Arena->Memory = New;
    }

    memcpy((cim_u8*)Arena->Memory + Arena->At, Data, Size);
    Arena->At += Size;
}

void *
CimArena_Push(size_t Size, cim_arena *Arena)
{
    if (Arena->At + Size > Arena->Capacity)
    {
        Arena->Capacity = Arena->Capacity ? Arena->Capacity * 2 : 1024;

        while (Arena->Capacity < Arena->At + Size)
        {
            Arena->Capacity += (Arena->Capacity >> 1);
        }

        void *New = malloc(Arena->Capacity);
        if (!New)
        {
            Cim_Assert(!"Malloc failure: OOM?");
            return NULL;
        }
        else
        {
            memset((char *)New + Arena->At, 0, Arena->Capacity - Arena->At);
        }

        if (Arena->Memory)
        {
            memcpy(New, Arena->Memory, Arena->At);
            free(Arena->Memory);
        }

        Arena->Memory = New;
    }

    void *Ptr = (char *)Arena->Memory + Arena->At;
    Arena->At += Size;

    return Ptr;
}

void *
CimArena_GetLast(size_t TypeSize, cim_arena *Arena)
{
    Cim_Assert(Arena->Memory);
    return (char *)Arena->Memory + (Arena->At - TypeSize);
}

cim_u32
CimArena_GetCount(size_t TypeSize, cim_arena *Arena)
{
    return (cim_u32)(Arena->At / TypeSize);
}

void
CimArena_Reset(cim_arena *Arena)
{
    Arena->At = 0;
}

void
CimArena_End(cim_arena *Arena)
{
    if (Arena->Memory)
    {
        free(Arena->Memory);

        Arena->Memory = NULL;
        Arena->At = 0;
        Arena->Capacity = 0;
    }
}

// [Render Commands: Implementation]

cim_draw_command *
GetDrawCommand(cim_command_buffer *Buffer)
{
    cim_draw_command *Command = NULL;

    if (Buffer->CommandCount == Buffer->CommandSize)
    {
        Buffer->CommandSize = Buffer->CommandSize ? Buffer->CommandSize * 2 : 32;

        cim_draw_command *New = (cim_draw_command*)malloc(Buffer->CommandSize * sizeof(cim_draw_command));
        if (!New)
        {
            CimLog_Fatal("Failed to heap-allocate command buffer.");
            return NULL;
        }

        if (Buffer->Commands)
        {
            memcpy(New, Buffer->Commands, Buffer->CommandCount * sizeof(cim_draw_command));
            free(Buffer->Commands);
        }

        memset(New, 0, Buffer->CommandSize * sizeof(cim_draw_command));
        Buffer->Commands = New;
    }

    if(Buffer->ClippingRectChanged || Buffer->FeatureStateChanged)
    {
        Command = Buffer->Commands + Buffer->CommandCount++;

        Command->VtxOffset = Buffer->FrameVtx.At;
        Command->IdxOffset = Buffer->FrameIdx.At;
        Command->VtxCount  = 0;
        Command->IdxCount  = 0;

        Buffer->ClippingRectChanged = false;
        Buffer->FeatureStateChanged = false;
    }
    else
    {
        Command = Buffer->Commands + (Buffer->CommandCount - 1);
    }

    return Command;
}

// NOTE: Have to profile which is better: Directly upload, or defer the upload
// to the renderer (More like a command type structure). Maybe we get better
// cache on the points? Have to profile.

void
DrawQuadFromData(cim_f32 x0, cim_f32 y0, cim_f32 x1, cim_f32 y1, cim_vector4 Col)
{
    Cim_Assert(CimCurrent);

    typedef struct local_vertex
    {
        cim_f32 PosX, PosY;
        cim_f32 U, V;
        cim_f32 R, G, B, A;
    } local_vertex;

    cim_command_buffer *Buffer  = UIP_COMMANDS;
    cim_draw_command   *Command = GetDrawCommand(Buffer);

    local_vertex Vtx[4];
    Vtx[0] = (local_vertex){ x0, y0, 0.0f, 1.0f, Col.x, Col.y, Col.z, Col.w };
    Vtx[1] = (local_vertex){ x0, y1, 0.0f, 0.0f, Col.x, Col.y, Col.z, Col.w };
    Vtx[2] = (local_vertex){ x1, y0, 1.0f, 1.0f, Col.x, Col.y, Col.z, Col.w };
    Vtx[3] = (local_vertex){ x1, y1, 1.0f, 0.0f, Col.x, Col.y, Col.z, Col.w };

    cim_u32 Idx[6];
    Idx[0] = Command->VtxCount + 0;
    Idx[1] = Command->VtxCount + 2;
    Idx[2] = Command->VtxCount + 1;
    Idx[3] = Command->VtxCount + 2;
    Idx[4] = Command->VtxCount + 3;
    Idx[5] = Command->VtxCount + 1;

    WriteToArena(Vtx, sizeof(Vtx), &Buffer->FrameVtx);
    WriteToArena(Idx, sizeof(Idx), &Buffer->FrameIdx);;

    Command->VtxCount += 4;
    Command->IdxCount += 6;
}

void
DrawQuadFromNode(cim_layout_node *Node, cim_vector4 Col)
{
    Cim_Assert(CimCurrent);

    typedef struct local_vertex
    {
        cim_f32 PosX, PosY;
        cim_f32 U, V;
        cim_f32 R, G, B, A;
    } local_vertex;

    cim_command_buffer *Buffer  = UIP_COMMANDS;
    cim_draw_command   *Command = GetDrawCommand(Buffer);

    cim_f32 x0 = Node->X;
    cim_f32 y0 = Node->Y;
    cim_f32 x1 = Node->X + Node->Width;
    cim_f32 y1 = Node->Y + Node->Height;

    local_vertex Vtx[4];
    Vtx[0] = (local_vertex){x0, y0, 0.0f, 1.0f, Col.x, Col.y, Col.z, Col.w};
    Vtx[1] = (local_vertex){x0, y1, 0.0f, 0.0f, Col.x, Col.y, Col.z, Col.w};
    Vtx[2] = (local_vertex){x1, y0, 1.0f, 1.0f, Col.x, Col.y, Col.z, Col.w};
    Vtx[3] = (local_vertex){x1, y1, 1.0f, 0.0f, Col.x, Col.y, Col.z, Col.w};

    cim_u32 Idx[6];
    Idx[0] = Command->VtxCount + 0;
    Idx[1] = Command->VtxCount + 2;
    Idx[2] = Command->VtxCount + 1;
    Idx[3] = Command->VtxCount + 2;
    Idx[4] = Command->VtxCount + 3;
    Idx[5] = Command->VtxCount + 1;

    WriteToArena(Vtx, sizeof(Vtx), &Buffer->FrameVtx);
    WriteToArena(Idx, sizeof(Idx), &Buffer->FrameIdx);;

    Command->VtxCount += 4;
    Command->IdxCount += 6;
}

// [Layout Tree]

void
PopParent()
{
    Cim_Assert(CimCurrent);

    cim_layout      *Layout = UIP_LAYOUT;
    cim_layout_tree *Tree   = &Layout->Tree; // This would access the current tree.

    if(Tree->AtParent > 0)
    {
        cim_u32 IdToPop = Tree->Nodes[Tree->ParentStack[Tree->AtParent--]].Id;
        if (IdToPop == Tree->FirstDragNode)
        {
            Tree->DragTransformX = 0;
            Tree->DragTransformY = 0;
        }
    }
    else
    {
        CimLog_Error("Unable to pop parent since we are already at depth 0");
    }
}

cim_layout_node *
GetNodeFromIndex(cim_u32 Index)
{
    Cim_Assert(CimCurrent);

    cim_layout      *Layout = UIP_LAYOUT;
    cim_layout_tree *Tree   = &Layout->Tree; // This would access the current tree.

    if (Index < Tree->NodeCount)
    {
        cim_layout_node *Node = Tree->Nodes + Index;

        return Node;
    }
    else
    {
        CimLog_Error("Invalid index used when accesing layout node: %u", Index);
        return NULL;
    }
}

cim_layout_node *
PushLayoutNode(bool IsContainer, cim_u32 *OutNodeId) // NOTE: Probably shouldn't take this as an argument.
{
    Cim_Assert(CimCurrent);

    cim_layout      *Layout = UIP_LAYOUT;
    cim_layout_tree *Tree   = &Layout->Tree; // This would access the current tree.

    if(Tree->NodeCount == Tree->NodeSize)
    {
        size_t NewSize = Tree->NodeSize ? (size_t)Tree->NodeSize * 2u : 8u;

        if (NewSize > (SIZE_MAX / sizeof(*Tree->Nodes))) 
        {
            CimLog_Fatal("Requested allocation size too large.");
            return NULL;
        }

        cim_layout_node *Temp = (cim_layout_node *)realloc(Tree->Nodes, NewSize * sizeof(*Temp));
        if (!Temp) 
        {
            CimLog_Fatal("Failed to heap-allocate.");
            return NULL;
        }

        Tree->Nodes    = Temp;
        Tree->NodeSize = (cim_u32)NewSize;
    }

    cim_u32          NodeId = Tree->NodeCount;
    cim_layout_node *Node   = Tree->Nodes + NodeId;
    cim_u32          Parent = Tree->ParentStack[Tree->AtParent];

    Node->Id          = NodeId;
    Node->FirstChild  = CimLayout_InvalidNode;
    Node->LastChild   = CimLayout_InvalidNode;
    Node->NextSibling = CimLayout_InvalidNode;
    Node->Parent      = CimLayout_InvalidNode;
    Node->Clicked     = false;
    Node->Held        = false;

    if(Parent != CimLayout_InvalidNode)
    {
        cim_layout_node *ParentNode = Tree->Nodes + Parent;

        if(ParentNode->FirstChild == CimLayout_InvalidNode)
        {
            ParentNode->FirstChild = NodeId;
            ParentNode->LastChild  = NodeId;
        }
        else
        {
            cim_layout_node *LastChild = Tree->Nodes + ParentNode->LastChild;
            LastChild->NextSibling = NodeId;
            ParentNode->LastChild  = NodeId;
        }

        Node->Parent = Parent;
    }

    if(IsContainer)
    {
        if(Tree->AtParent + 1 < CimLayout_MaxNestDepth)
        {
            Tree->ParentStack[++Tree->AtParent] = NodeId;
        }
        else
        {
            CimLog_Error("Maximum nest depth reached: %u", CimLayout_MaxNestDepth);
        }
    }

    ++Tree->NodeCount;

    *OutNodeId = NodeId;

    return Node;
}

cim_component *
FindComponent(const char *Key)
{
    Cim_Assert(CimCurrent);
    cim_component_hashmap *Hashmap = &UI_LAYOUT.Components;

    if (!Hashmap->IsInitialized)
    {
        Hashmap->GroupCount = 32;

        cim_u32 BucketCount  = Hashmap->GroupCount * CimBucketGroupSize;
        size_t  BucketSize   = BucketCount * sizeof(cim_component_entry);
        size_t  MetadataSize = BucketCount * sizeof(cim_u8);

        Hashmap->Buckets  = (cim_component_entry*)malloc(BucketSize);
        Hashmap->Metadata = (cim_u8*)malloc(MetadataSize);

        if (!Hashmap->Buckets || !Hashmap->Metadata)
        {
            return NULL;
        }

        memset(Hashmap->Buckets, 0, BucketSize);
        memset(Hashmap->Metadata, CimEmptyBucketTag, MetadataSize);

        Hashmap->IsInitialized = true;
    }

    cim_u32 ProbeCount = 0;
    cim_u32 Hashed     = CimHash_String32(Key);
    cim_u32 GroupIdx   = Hashed & (Hashmap->GroupCount - 1);

    while (true)
    {
        cim_u8 *Meta = Hashmap->Metadata + GroupIdx * CimBucketGroupSize;
        cim_u8  Tag = (Hashed & 0x7F);

        __m128i Mv = _mm_loadu_si128((__m128i *)Meta);
        __m128i Tv = _mm_set1_epi8(Tag);

        cim_i32 TagMask = _mm_movemask_epi8(_mm_cmpeq_epi8(Mv, Tv));
        while (TagMask)
        {
            cim_u32 Lane = CimHash_FindFirstBit32(TagMask);
            cim_u32 Idx = (GroupIdx * CimBucketGroupSize) + Lane;

            cim_component_entry *E = Hashmap->Buckets + Idx;
            if (strcmp(E->Key, Key) == 0)
            {
                return &E->Value;
            }

            TagMask &= TagMask - 1;
        }

        __m128i Ev        = _mm_set1_epi8(CimEmptyBucketTag);
        cim_i32 EmptyMask = _mm_movemask_epi8(_mm_cmpeq_epi8(Mv, Ev));
        if (EmptyMask)
        {
            cim_u32 Lane = CimHash_FindFirstBit32(EmptyMask);
            cim_u32 Idx = (GroupIdx * CimBucketGroupSize) + Lane;

            Meta[Lane] = Tag;

            cim_component_entry *E = Hashmap->Buckets + Idx;
            strcpy_s(E->Key, sizeof(E->Key), Key);
            E->Key[sizeof(E->Key) - 1] = 0;

            return &E->Value;
        }

        ProbeCount++;
        GroupIdx = (GroupIdx + ProbeCount * ProbeCount) & (Hashmap->GroupCount - 1);
    }

    return NULL;
}

// [Styles]

valid_component ValidComponents[] =
{
    {"Window", (sizeof("Window") - 1), CimComponent_Window},
    {"Button", (sizeof("Button") - 1), CimComponent_Button},
};

valid_attribute ValidAttributes[] =
{
    {"Size", (sizeof("Size") - 1), Attribute_Size,
     CimComponent_Window | CimComponent_Button},

    {"Color", (sizeof("Color") - 1), Attribute_Color,
     CimComponent_Window | CimComponent_Button},

    {"Padding", (sizeof("Padding") - 1), Attribute_Padding,
     CimComponent_Window},

    {"Spacing", (sizeof("Spacing") - 1), Attribute_Spacing,
     CimComponent_Window},

    {"LayoutOrder", (sizeof("LayoutOrder") - 1), Attribute_LayoutOrder,
     CimComponent_Window},

    {"BorderColor", (sizeof("BorderColor") - 1), Attribute_BorderColor,
     CimComponent_Window | CimComponent_Button},

    {"BorderWidth", (sizeof("BorderWidth") - 1), Attribute_BorderWidth,
     CimComponent_Window | CimComponent_Button},
};

buffer
ReadEntireFile(const char *File)
{
    buffer Result = {};

    FILE *FilePointer;
    fopen_s(&FilePointer, File, "rb");
    if (!FilePointer)
    {
        CimLog_Error("Failed to open the styling file | for: %s", File);
        return Result;
    }
    if (fseek(FilePointer, 0, SEEK_END) != 0)
    {
        CimLog_Error("Failed to seek the EOF | for: %s", File);
        fclose(FilePointer);
        return Result;
    }

    Result.Size = (cim_u32)ftell(FilePointer);
    if (Result.Size < 0)
    {
        CimLog_Error("Failed to tell the size of the file | for: %s", File);
        return Result;
    }

    fseek(FilePointer, 0, SEEK_SET);

    Result.Data = (cim_u8*)malloc((size_t)Result.Size);
    if (!Result.Data)
    {
        CimLog_Error("Failed to heap-allocate | for: %s, with size: %d", File, Result.Size);

        free(Result.Data);
        Result.Data = NULL;

        fclose(FilePointer);

        return Result;
    }

    size_t Read = fread(Result.Data, 1, (size_t)Result.Size, FilePointer);
    if (Read != (size_t)Result.Size)
    {
        CimLog_Error("Could not read the full file | for: %s", File);

        free(Result.Data);
        Result.Data = NULL;

        fclose(FilePointer);

        return Result;
    }

    fclose(FilePointer);

    return Result;
}

token *
CreateToken(Token_Type Type, lexer *Lexer)
{
    if (Lexer->TokenCount == Lexer->TokenCapacity)
    {
        Lexer->TokenCapacity = Lexer->TokenCapacity ? Lexer->TokenCapacity * 2 : 64;

        token *New = (token*)malloc(Lexer->TokenCapacity * sizeof(token));
        if (!New)
        {
            CimLog_Error("Failed to heap-allocate a buffer for the tokens. OOM?");
            return NULL;
        }

        memcpy(New, Lexer->Tokens, Lexer->TokenCount * sizeof(token));
        free(Lexer->Tokens);

        Lexer->Tokens = New;
    }

    token *Token = Lexer->Tokens + Lexer->TokenCount++;
    Token->Type = Type;

    return Token;
}

bool
IsAlphaCharacter(cim_u8 Character)
{
    bool Result = (Character >= 'A' && Character <= 'Z') ||
        (Character >= 'a' && Character <= 'z');

    return Result;
}

bool
IsNumberCharacter(cim_u8 Character)
{
    bool Result = (Character >= '0' && Character <= '9');

    return Result;
}

lexer
CreateTokenStreamFromBuffer(buffer *Content)
{
    lexer Lexer = {};
    Lexer.Tokens        = (token*)malloc(1000 * sizeof(token));
    Lexer.TokenCapacity = 1000;
    Lexer.IsValid       = false;

    if (!Content->Data || !Content->Size)
    {
        CimLog_Error("Error parsing file: The file is empty or wasn't read.");
        return Lexer;
    }

    if (!Lexer.Tokens)
    {
        CimLog_Error("Failed to heap-allocate. Out of memory?");
        return Lexer;
    }

    while (Content->At < Content->Size)
    {
        while (Content->Data[Content->At] == ' ')
        {
            Content->At += 1;
        }

        cim_u8 *Character = Content->Data + Content->At;
        cim_u32 StartAt   = Content->At;
        cim_u32 At        = StartAt;

        if (IsAlphaCharacter(*Character))
        {
            cim_u8 *Identifier = Content->Data + At;
            while (At < Content->Size && IsAlphaCharacter(*Identifier))
            {
                Identifier++;
            }

            token *Token = CreateToken(Token_Identifier, &Lexer);
            Token->Name = Character;
            Token->NameLength = (cim_u32)(Identifier - Character);

            At += Token->NameLength;
        }
        else if (IsNumberCharacter(*Character))
        {
            cim_u32 Number = 0;
            while (At < Content->Size && IsNumberCharacter(Content->Data[At]))
            {
                Number = (Number * 10) + (*Character - '0');
                At += 1;
            }

            token *Token = CreateToken(Token_Number, &Lexer);
            Token->UInt32 = Number;
        }
        else if (*Character == '\r' || *Character == '\n')
        {
            //  NOTE: Doesn't do much right now, but will be used to provide better error messages.

            cim_u8 C = Content->Data[At++];
            if (C == '\r' && Content->Data[At] == '\n')
            {
                At++;
            }
        }
        else if (*Character == ':')
        {
            At++;

            if (At < Content->Size && Content->Data[At] == '=')
            {
                CreateToken(Token_Assignment, &Lexer);
                At++;
            }
            else
            {
                CimLog_Error("Stray ':' token. Did you mean := (Assignment)?");
                return Lexer;
            }
        }
        else if (*Character == '[')
        {
            // NOTE: The formatting rules are quite strict. And weird regarding
            // whitespaces.

            At++;

            cim_f32 Vector[4] = { 0 };
            cim_u32 DigitCount = 0;
            while (DigitCount < 4 && Content->Data[At] != ';')
            {
                while (At < Content->Size && Content->Data[At] == ' ')
                {
                    At++;
                }

                cim_u32 Number = 0;
                while (At < Content->Size && IsNumberCharacter(Content->Data[At]))
                {
                    Number = (Number * 10) + (Content->Data[At] - '0');
                    At += 1;
                }

                if (Content->Data[At] != ',' && Content->Data[At] != ']')
                {
                    CimLog_Error("...");
                    return Lexer;
                }

                At++;
                Vector[DigitCount++] = Number;
            }

            token *Token = CreateToken(Token_Vector, &Lexer);
            memcpy(Token->Vector, Vector, sizeof(Token->Vector));
            Token->VectorSize = DigitCount;
        }
        else if (*Character == '"')
        {
            At++;

            token *Token = CreateToken(Token_String, &Lexer);
            Token->Name = Content->Data + At;

            while (IsAlphaCharacter(Content->Data[At]))
            {
                At++;
            }

            if (Content->Data[At] != '"')
            {
                CimLog_Error("...");
                return Lexer;
            }

            Token->NameLength = (cim_u32)((Content->Data + At++) - Token->Name);
        }
        else if (*Character == '#')
        {
            At++;

            cim_f32 Vector[4]  = { 0.0f };
            cim_i32 VectorIdx  = 0;
            cim_f32 Inverse    = 1.0f / 255.0f;
            cim_u32 MaximumHex = 8; // (#RRGGBBAA)
            cim_u32 HexCount   = 0;

            while ((HexCount + 1) < MaximumHex && VectorIdx < 4)
            {
                cim_u32 Value = 0;
                bool    Valid = true;
                for (cim_u32 Idx = 0; Idx < 2; Idx++)
                {
                    char    C = Content->Data[At + HexCount + Idx];
                    cim_u32 Digit = 0;

                    if      (C >= '0' && C <= '9') Digit = C - '0';
                    else if (C >= 'A' && C <= 'F') Digit = C - 'A' + 10;
                    else if (C >= 'a' && C <= 'f') Digit = C - 'a' + 10;
                    else { Valid = false; break; }

                    Value = (Value << 4) | Digit;
                }

                if (!Valid) break;

                Vector[VectorIdx++] = Value * Inverse;
                HexCount           += 2;
            }

            At += HexCount;

            if (IsNumberCharacter(Content->Data[At]) || IsAlphaCharacter(Content->Data[At]))
            {
                CimLog_Error("...");
                return Lexer;
            }

            if (HexCount == 6)
            {
                Vector[3] = 1.0f;
            }

            token *Token = CreateToken(Token_Vector, &Lexer);
            memcpy(Token->Vector, Vector, sizeof(Token->Vector));
            Token->VectorSize = 4;
        }
        else
        {
            // WARN: Still unsure.
            CreateToken((Token_Type)*Character, &Lexer);
            At++;
        }

        Content->At = At;
    }

    Lexer.IsValid = true;
    return Lexer;
}

// TODO: 
// 1) Improve the error messages & Continue implementing the logic.

user_styles
CreateUserStyles(lexer *Lexer)
{
    user_styles Styles = {};
    Styles.Descs    = (style_desc*)calloc(10, sizeof(style_desc));
    Styles.DescSize = 100;
    Styles.IsValid  = false;

    cim_u32 AtToken = 0;
    while (AtToken < Lexer->TokenCount)
    {
        token *Token = Lexer->Tokens + AtToken;
        AtToken     += 1;

        switch (Token->Type)
        {

        case Token_String:
        {
            token *Next = Lexer->Tokens + AtToken;
            AtToken += 1;

            if (Next->Type != Token_Assignment)
            {
                CimLog_Error("...");
                return Styles;
            }

            Next = Lexer->Tokens + AtToken;
            AtToken += 1;

            if (Next->Type != Token_Identifier)
            {
                CimLog_Error("...");
                return Styles;
            }

            bool IsValidComponentName = false;
            for (cim_u32 ValidIdx = 0; ValidIdx < CIM_ARRAY_COUNT(ValidComponents); ValidIdx++)
            {
                valid_component *Valid = ValidComponents + ValidIdx;

                if (Valid->Length != Next->NameLength)
                {
                    continue;
                }

                if (memcmp(Next->Name, Valid->Name, Next->NameLength) == 0)
                {
                    // BUG: Does not check for overflows.
                    style_desc *Desc = Styles.Descs + Styles.DescCount++;

                    memcpy(Desc->Id, Token->Name, Token->NameLength);
                    Desc->ComponentFlag = Valid->ComponentFlag;

                    IsValidComponentName = true;

                    break;
                }
            }

            if (!IsValidComponentName)
            {
                CimLog_Error("...");
                return Styles;
            }
        } break;

        case Token_Identifier:
        {
            valid_attribute Attr = {};
            bool            Found = false;

            style_desc *Desc = Styles.Descs + (Styles.DescCount - 1);
            if (Styles.DescCount == 0 || Desc->ComponentFlag == CimComponent_Invalid || !Desc)
            {
                CimLog_Error("...");
                return Styles;
            }

            for (cim_u32 Idx = 0; Idx < CIM_ARRAY_COUNT(ValidAttributes); Idx++)
            {
                valid_attribute Valid = ValidAttributes[Idx];

                if (Valid.Length != Token->NameLength || !(Valid.ComponentFlag & Desc->ComponentFlag))
                {
                    continue;
                }

                if (memcmp(Valid.Name, Token->Name, Token->NameLength) == 0)
                {
                    Attr = Valid;
                    Found = true;

                    break;
                }
            }

            if (!Found)
            {
                CimLog_Error("...");
                return Styles;
            }

            token *Next = Lexer->Tokens + AtToken;
            AtToken += 1;

            if (Next->Type != Token_Assignment)
            {
                CimLog_Error("...");
                return Styles;
            }

            Next = Lexer->Tokens + AtToken;
            AtToken += 1;

            bool IsNegative = false;
            if (Next->Type == '-')
            {
                IsNegative = true;

                Next = Lexer->Tokens + AtToken;
                AtToken += 1;
            }

            // NOTE: Can I not compress this? Structure seems a bit bad?
            // Something is weird with this data flow.

            switch (Attr.Type)
            {

            case Attribute_Size:
            {
                FAIL_ON_NEGATIVE(IsNegative, "Value cannot be negative.");
                ARRAY_TO_VECTOR2(Next->Vector, Next->VectorSize, Desc->Style.Size)
            } break;

            case Attribute_Color:
            {
                FAIL_ON_NEGATIVE(IsNegative, "Value cannot be negative.");
                ARRAY_TO_VECTOR4(Next->Vector, Next->VectorSize, Desc->Style.Color)
            } break;

            case Attribute_Spacing:
            {
                FAIL_ON_NEGATIVE(IsNegative, "Value cannot be negative.");
                ARRAY_TO_VECTOR2(Next->Vector, Next->VectorSize, Desc->Style.Spacing)
            } break;

            case Attribute_Padding:
            {
                FAIL_ON_NEGATIVE(IsNegative, "Value cannot be negative.");
                ARRAY_TO_VECTOR4(Next->Vector, Next->VectorSize, Desc->Style.Padding)
            } break;

            case Attribute_LayoutOrder:
            {
                FAIL_ON_NEGATIVE(IsNegative, "Value cannot be negative.");
                if(Next->NameLength == 10) // Hack because lazy
                {
                    Desc->Style.Order = Layout_Horizontal;
                }
                else
                {
                    Desc->Style.Order = Layout_Vertical;
                }
            } break;

            case Attribute_BorderColor:
            {
                FAIL_ON_NEGATIVE(IsNegative, "Value cannot be negative.");
                ARRAY_TO_VECTOR4(Next->Vector, Next->VectorSize, Desc->Style.BorderColor)
            } break;

            case Attribute_BorderWidth:
            {
                FAIL_ON_NEGATIVE(IsNegative, "Value cannot be negative.");
                Desc->Style.BorderWidth = Next->UInt32;
            } break;

            default:
            {
                CimLog_Error("...");
                return Styles;
            } break;

            }

            Next = Lexer->Tokens + AtToken;
            AtToken += 1;

            if (Next->Type != ';')
            {
                CimLog_Error("...");
                return Styles;
            }

        } break;

        default:
        {
            CimLog_Info("Skipping unknown token.");
        } break;

        }

    }

    Styles.IsValid = true;
    return Styles;
}

bool
CimStyle_Set(user_styles *Styles)
{
    for (cim_u32 DescIdx = 0; DescIdx < Styles->DescCount; DescIdx++)
    {
        style_desc    *Desc      = Styles->Descs + DescIdx;
        cim_component *Component = FindComponent(Desc->Id);

        switch (Desc->ComponentFlag)
        {

        case CimComponent_Window:
        {
            cim_window *Window = &Component->For.Window;

            // Style
            Window->Style.Color       = Desc->Style.Color;
            Window->Style.BorderColor = Desc->Style.BorderColor;
            Window->Style.BorderWidth = Desc->Style.BorderWidth;

            // Layout
            Window->Style.Size    = Desc->Style.Size;
            Window->Style.Order   = Desc->Style.Order;
            Window->Style.Padding = Desc->Style.Padding;
            Window->Style.Spacing = Desc->Style.Spacing;
        } break;

        case CimComponent_Button:
        {
            cim_button *Button = &Component->For.Button;

            // Style
            Button->Style.Color       = Desc->Style.Color;
            Button->Style.BorderColor = Desc->Style.BorderColor;
            Button->Style.BorderWidth = Desc->Style.BorderWidth;

            // Layout
            Button->Style.Size = Desc->Style.Size;
        } break;

        case CimComponent_Invalid:
        {
            CimLog_Error("...");
            return false;
        }

        }
    }

    return true;
}

void
CimStyle_Initialize(const char *File)
{
    buffer      FileContent = {};
    lexer       Lexer       = {};
    user_styles Styles      = {};

    FileContent = ReadEntireFile(File);
    if (!FileContent.Data)
    {
        CimLog_Fatal("Failed at: Reading file. See Error(s) Above");
        return;
    }

    Lexer = CreateTokenStreamFromBuffer(&FileContent);
    if (!Lexer.IsValid)
    {
        goto Cleanup;
        CimLog_Fatal("Failed at: Creating token stream. See Error(s) Above");
        return;
    }

    Styles = CreateUserStyles(&Lexer);
    if (!Styles.IsValid)
    {
        goto Cleanup;
        CimLog_Fatal("Failed at: Creating user styles. See Error(s) Above");
        return;
    }

    if (!CimStyle_Set(&Styles))
    {
        goto Cleanup;
        CimLog_Fatal("Failed at: Setting user styles. See Error(s) Above.");
        return;
    }

Cleanup:
    if (FileContent.Data) free(FileContent.Data);
    if (Lexer.Tokens)     free(Lexer.Tokens);
    if (Styles.Descs)     free(Styles.Descs);
}

// NOTE: This is a first step to moving to a unity build.
#include "cim_text.cpp"

// [Public API] =============================

#define _UI_CONCAT2(a,b) a##b
#define _UI_CONCAT(a,b) _UI_CONCAT2(a,b)
#define _UI_UNIQUE(name) _UI_CONCAT(name, __LINE__)

#define UIWindow(Id, Flags)                                                              \
    for (int _UI_UNIQUE(_ui_iter) = 0; _UI_UNIQUE(_ui_iter) < 2; ++_UI_UNIQUE(_ui_iter)) \
        if (Cim_Window(Id, Flags))                                                       \
            for (int _UI_UNIQUE(_ui_once) = 1; _UI_UNIQUE(_ui_once);                     \
                 (Cim_EndWindow(), _UI_UNIQUE(_ui_once) = 0))


#define UIButton(Id, ...)                                         \
    for (struct { bool _clicked; int _once; } _UI_UNIQUE(_ui) = { \
              ._clicked = Cim_Button((Id)), ._once = 1 };         \
         _UI_UNIQUE(_ui)._once;                                   \
         (_UI_UNIQUE(_ui)._once = 0)                              \
        )                                                         \
        if (_UI_UNIQUE(_ui)._clicked __VA_ARGS__)

typedef enum CimWindow_Flags
{
    CimWindow_AllowDrag = 1 << 0,
} CimWindow_Flags;

bool
Cim_Window(const char *Id, cim_bit_field Flags)
{
    Cim_Assert(CimCurrent);
    CimContext_State State = UI_STATE;

    if (State == CimContext_Layout)
    {
        cim_component   *Component = FindComponent(Id);
        cim_window      *Window    = &Component->For.Window;

        if (!Window->IsInitialized)
        {
            Window->LastFrameScreenX = 500;
            Window->LastFrameScreenY = 500;
            Window->IsInitialized = true;
        }

        cim_layout_node *Node = PushLayoutNode(true, &Component->LayoutNodeIndex);
        Node->PrefWidth  = Window->Style.Size.x;
        Node->PrefHeight = Window->Style.Size.y;
        Node->Padding    = Window->Style.Padding;
        Node->Spacing    = Window->Style.Spacing;
        Node->Order      = Window->Style.Order;
        Node->X          = Window->LastFrameScreenX;
        Node->Y          = Window->LastFrameScreenY;

        return true; // Need to find a way to cache some state. E.g. closed and not hovering
    }
    else if (State == CimContext_Interaction)
    {
        UI_COMMANDS.ClippingRectChanged = true; // Uhm.. Probably should not even be a direct set, but more of a check.

        cim_component   *Component = FindComponent(Id);                            // This is the second access to the hashmap.
        cim_window      *Window    = &Component->For.Window;
        cim_layout_node *Node      = GetNodeFromIndex(Component->LayoutNodeIndex); // Can't we just call get next node since it's the same order? Same for hashmap?

        if (Flags & CimWindow_AllowDrag)
        {
            if (Node->Held)
            {
                cim_layout_tree *Tree = UIP_LAYOUT.Tree;          // NOTE: Should somehow access our own tree.
                Tree->DragTransformX = GetMouseDeltaX(UIP_INPUT);
                Tree->DragTransformY = GetMouseDeltaY(UIP_INPUT);

                Node->X += Tree->DragTransformX;
                Node->Y += Tree->DragTransformY;
            }
        }

        // Duplicate code
        if (Window->Style.BorderWidth > 0)
        {
            cim_f32 x0 = Node->X - Window->Style.BorderWidth;
            cim_f32 y0 = Node->Y - Window->Style.BorderWidth;
            cim_f32 x1 = Node->X + Node->Width  + Window->Style.BorderWidth;
            cim_f32 y1 = Node->Y + Node->Height + Window->Style.BorderWidth;
            DrawQuadFromData(x0, y0, x1, y1, Window->Style.BorderColor);
        }
        DrawQuadFromNode(Node, Window->Style.Color);

        Window->LastFrameScreenX = Node->X;
        Window->LastFrameScreenY = Node->Y;

        return true;
    }
    else
    {
        CimLog_Error("Invalid state");
        return false;
    }
}

bool
Cim_Button(const char *Id)
{
    Cim_Assert(CimCurrent);
    CimContext_State State = UI_STATE;

    if(State == CimContext_Layout)
    {
        cim_component   *Component = FindComponent(Id);
        cim_button_style Style     = Component->For.Button.Style;

        cim_layout_node *Node = PushLayoutNode(false, &Component->LayoutNodeIndex);
        Node->ContentWidth  = Style.Size.x;
        Node->ContentHeight = Style.Size.y;

        return IsMouseDown(CimMouse_Left, UIP_INPUT);
    }
    else if(State == CimContext_Interaction) // Maybe rename this state then?
    {
        cim_layout_tree *Tree      = UIP_LAYOUT.Tree;                              // Another global access.
        cim_component   *Component = FindComponent(Id);                            // This is the second access to the hashmap.
        cim_button_style Style     = Component->For.Button.Style;
        cim_layout_node *Node      = GetNodeFromIndex(Component->LayoutNodeIndex); // Can't we just call get next node since it's the same order? Same for hashmap?

        if (Node->Clicked)
        {
            Tree->DragTransformX = 0;
            Tree->DragTransformY = 0;
        }
        else
        {
            Node->X += Tree->DragTransformX;
            Node->Y += Tree->DragTransformY;
        }

        // NOW: Do we draw here? We can for now.

        if (Style.BorderWidth > 0)
        {
            cim_f32 x0 = Node->X - Style.BorderWidth;
            cim_f32 y0 = Node->Y - Style.BorderWidth;
            cim_f32 x1 = Node->X + Node->Width  + Style.BorderWidth;
            cim_f32 y1 = Node->Y + Node->Height + Style.BorderWidth;
            DrawQuadFromData(x0, y0, x1, y1, Style.BorderColor);
        }
        DrawQuadFromNode(Node, Style.Color);

        return Node->Clicked;
    }
    else
    {
        CimLog_Error("Invalid state");
        return false;
    }
}

void
Cim_EndWindow()
{
    Cim_Assert(CimCurrent);
    cim_layout_tree *Tree  = UIP_LAYOUT.Tree;
    CimContext_State State = UI_STATE;

    if(State == CimContext_Layout)
    {
        Tree->DragTransformX = 0;
        Tree->DragTransformY = 0;

        cim_u32 DownUpStack[1024] = {};
        cim_u32 StackAt           = 0;
        cim_u32 NodeCount         = 0;
        char    Visited[512]      = {};

        DownUpStack[StackAt++] = 0;

        while (StackAt > 0)
        {
            cim_u32 Current = DownUpStack[StackAt - 1];

            if (!Visited[Current])
            {
                Visited[Current] = 1;

                cim_u32 Child = Tree->Nodes[Current].FirstChild;
                while (Child != CimLayout_InvalidNode)
                {
                    DownUpStack[StackAt++] = Child;
                    Child = Tree->Nodes[Child].NextSibling;
                }
            }
            else
            {
                StackAt--;
                NodeCount++;

                cim_layout_node *Node = Tree->Nodes + Current;
                if (Node->FirstChild == CimLayout_InvalidNode)
                {
                    Node->PrefWidth  = Node->ContentWidth;
                    Node->PrefHeight = Node->ContentHeight;
                }
                else
                {
                    cim_u32 ChildCount = 0;

                    if (Node->Order == Layout_Horizontal)
                    {
                        cim_f32 MaximumHeight = 0;
                        cim_f32 TotalWidth = 0;

                        cim_u32 Child = Tree->Nodes[Current].FirstChild;
                        while (Child != CimLayout_InvalidNode)
                        {
                            if (Tree->Nodes[Child].PrefHeight > MaximumHeight)
                            {
                                MaximumHeight = Tree->Nodes[Child].PrefHeight;
                            }

                            TotalWidth += Tree->Nodes[Child].PrefWidth;
                            ChildCount += 1;

                            Child = Tree->Nodes[Child].NextSibling;
                        }

                        cim_f32 Spacing = (ChildCount > 1) ? Node->Spacing.x * (ChildCount - 1) : 0.0f;

                        Node->PrefWidth = TotalWidth + (Node->Padding.x + Node->Padding.z) + Spacing;
                        Node->PrefHeight = MaximumHeight + (Node->Padding.y + Node->Padding.w);
                    }
                    else if (Node->Order == Layout_Vertical)
                    {
                        cim_f32 MaximumWidth = 0;
                        cim_f32 TotalHeight = 0;

                        cim_u32 Child = Tree->Nodes[Current].FirstChild;
                        while (Child != CimLayout_InvalidNode)
                        {
                            if (Tree->Nodes[Child].PrefWidth > MaximumWidth)
                            {
                                MaximumWidth = Tree->Nodes[Child].PrefWidth;
                            }

                            TotalHeight += Tree->Nodes[Child].PrefHeight;
                            ChildCount += 1;

                            Child = Tree->Nodes[Child].NextSibling;
                        }

                        cim_f32 Spacing = (ChildCount > 1) ? Node->Spacing.y * (ChildCount - 1) : 0.0f;

                        Node->PrefWidth = MaximumWidth + (Node->Padding.x + Node->Padding.z);
                        Node->PrefHeight = TotalHeight + (Node->Padding.y + Node->Padding.w) + Spacing;
                    }
                }

            }
        }

        cim_u32 UpDownStack[1024] = {};
        StackAt                   = 0;
        UpDownStack[StackAt++]    = 0;

        // How to place the window is still an issue.
        Tree->Nodes[0].Width  = Tree->Nodes[0].PrefWidth;
        Tree->Nodes[0].Height = Tree->Nodes[0].PrefHeight;

        cim_f32 ClientX;
        cim_f32 ClientY;

        while (StackAt > 0)
        {
            cim_u32          Current = UpDownStack[--StackAt];
            cim_layout_node *Node    = Tree->Nodes + Current;

            ClientX = Node->X + Node->Padding.x;
            ClientY = Node->Y + Node->Padding.y;

            // TODO: Change how we iterate this.
            cim_u32 Child     = Node->FirstChild;
            cim_u32 Temp[256] = {};
            cim_u32 Idx       = 0;
            while (Child != CimLayout_InvalidNode)
            {
                cim_layout_node *CNode = Tree->Nodes + Child;

                CNode->X      = ClientX;
                CNode->Y      = ClientY;
                CNode->Width  = CNode->PrefWidth;  // Weird.
                CNode->Height = CNode->PrefHeight; // Weird.

                if (Node->Order == Layout_Horizontal)
                {
                    ClientX += CNode->Width + Node->Spacing.x;
                }
                else if (Node->Order == Layout_Vertical)
                {
                    ClientY += CNode->Height + Node->Spacing.y;
                }

                Temp[Idx++] = Child;
                Child = CNode->NextSibling;
            }

            for (cim_i32 TempIdx = (cim_i32)Idx - 1; TempIdx >= 0; --TempIdx)
            {
                UpDownStack[StackAt++] = Temp[TempIdx];
            }
        }

        UI_STATE = CimContext_Interaction;
        PopParent();

        bool MouseClicked = IsMouseClicked(CimMouse_Left, UIP_INPUT);
        bool MouseDown    = IsMouseDown(CimMouse_Left, UIP_INPUT);

        if(MouseClicked || MouseDown)
        {
            cim_layout_node *Root         = Tree->Nodes;
            cim_rect         WindowHitBox = MakeRectFromNode(Root);

            if (IsInsideRect(WindowHitBox))
            {
                for (cim_u32 StackIdx = NodeCount - 1; StackIdx > 0; StackIdx--)
                {
                    cim_layout_node *Node   = Tree->Nodes + DownUpStack[StackIdx];
                    cim_rect         HitBox = MakeRectFromNode(Node);

                    if (IsInsideRect(HitBox))
                    {
                        Node->Held    = MouseDown;
                        Node->Clicked = MouseClicked;
                        return;
                    }
                }

                Root->Held    = MouseDown;
                Root->Clicked = MouseClicked;
            }
        }
    }
    else if (State == CimContext_Interaction)
    {
        // Unsure. Maybe dragging?
    }
    else
    {
        CimLog_Error("Invalid State");
    }
}

// NOTE: This function is kind of weird.

void
BeginUIFrame()
{
    // Obviously wouldn't really do this but prototyping.
    cim_layout *Layout = UIP_LAYOUT;
    Layout->Tree.NodeCount      = 0;
    Layout->Tree.AtParent       = 0;
    Layout->Tree.ParentStack[0] = CimLayout_InvalidNode; // Temp.

    UI_STATE = CimContext_Layout;
}

void EndUIFrame()
{
    cim_inputs *Inputs = UIP_INPUT;
    Inputs->ScrollDelta = 0;
    Inputs->MouseDeltaX = 0;
    Inputs->MouseDeltaY = 0;

    for (cim_u32 Idx = 0; Idx < CimIO_KeyboardKeyCount; Idx++)
    {
        Inputs->Buttons[Idx].HalfTransitionCount = 0;
    }

    for (cim_u32 Idx = 0; Idx < CimMouse_ButtonCount; Idx++)
    {
        Inputs->MouseButtons[Idx].HalfTransitionCount = 0;
    }
}

// [Features]

typedef enum CimFeature_Type
{
    CimFeature_AlbedoMap   = 1 << 0,
    CimFeature_MetallicMap = 1 << 1,
    CimFeature_Count       = 2,
} CimFeature_Type;

#include "cim_platform.cpp"

// [Backends]

#ifdef _WIN32

#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>

#pragma comment (lib, "d3dcompiler")
#pragma comment (lib, "dxguid.lib")

#define CimDx11_Release(obj) if(obj) obj->Release(); obj = NULL;
#define Cim_AssertHR(hr) Cim_Assert((SUCCEEDED(hr)));

typedef struct cim_dx11_pipeline
{
    ID3D11InputLayout  *Layout;
    ID3D11VertexShader *VtxShader;
    ID3D11PixelShader  *PxlShader;

    UINT Stride;
    UINT Offset;
} cim_dx11_pipeline;

typedef struct cim_dx11_entry
{
    cim_bit_field     Key;
    cim_dx11_pipeline Value;
} cim_dx11_entry;

typedef struct cim_dx11_pipeline_hashmap
{
    cim_u8         *Metadata;
    cim_dx11_entry *Buckets;
    cim_u32         GroupCount;
    bool            IsInitialized;
} cim_dx11_pipeline_hashmap;

typedef struct cim_dx11_shared_data
{
    cim_f32 SpaceMatrix[4][4];
} cim_dx11_shared_data;

typedef struct cim_dx11_backend
{
    ID3D11Device        *Device;
    ID3D11DeviceContext *DeviceContext;

    ID3D11Buffer *VtxBuffer;
    ID3D11Buffer *IdxBuffer;
    size_t        VtxBufferSize;
    size_t        IdxBufferSize;

    ID3D11Buffer *SharedFrameData;

    cim_dx11_pipeline_hashmap PipelineStore;
} cim_dx11_backend;

const char *CimDx11_VertexShader =
"cbuffer PerFrame : register(b0)                                        \n"
"{                                                                      \n"
"    matrix SpaceMatrix;                                                \n"
"};                                                                     \n"
"                                                                       \n"
"struct VertexInput                                                     \n"
"{                                                                      \n"
"    float2 Pos : POSITION;                                             \n"
"    float2 Tex : NORMAL;                                               \n"
"    float4 Col : COLOR;                                                \n"
"};                                                                     \n"
"                                                                       \n"
"struct VertexOutput                                                    \n"
"{                                                                      \n"
"   float4 Position : SV_POSITION;                                      \n"
"   float4 Col      : COLOR;                                            \n"
"};                                                                     \n"
"                                                                       \n"
"VertexOutput VSMain(VertexInput Input)                                 \n"
"{                                                                      \n"
"    VertexOutput Output;                                               \n"
"                                                                       \n"
"    Output.Position = mul(SpaceMatrix, float4(Input.Pos, 1.0f, 1.0f)); \n"
"    Output.Col      = Input.Col;                                       \n"
"                                                                       \n"
"    return Output;                                                     \n"
"}                                                                      \n"
;

const char *CimDx11_PixelShader =
"struct PSInput                           \n"
"{                                        \n"
"    float4 Position : SV_POSITION;       \n"
"    float4 Col      : COLOR;             \n"
"};                                       \n"
"                                         \n"
"float4 PSMain(PSInput Input) : SV_TARGET \n"
"{                                        \n"
"    float4 Color = Input.Col;            \n"
"    return Color;                        \n"
"}                                        \n"
;

static ID3DBlob *
CimDx11_CompileShader(const char *ByteCode, size_t ByteCodeSize, const char *EntryPoint,
                      const char *Profile, D3D_SHADER_MACRO *Defines, UINT Flags)
{
    ID3DBlob *ShaderBlob = NULL;
    ID3DBlob *ErrorBlob  = NULL;

    HRESULT Status = D3DCompile(ByteCode, ByteCodeSize,
                                NULL, Defines, NULL,
                                EntryPoint, Profile, Flags, 0,
                                &ShaderBlob, &ErrorBlob);
    Cim_AssertHR(Status);

    return ShaderBlob;
}

static ID3D11VertexShader *
CimDx11_CreateVtxShader(D3D_SHADER_MACRO *Defines, ID3DBlob **OutShaderBlob)
{
    Cim_Assert(CimCurrent);

    cim_dx11_backend *Backend = (cim_dx11_backend *)CimCurrent->Backend;
    HRESULT           Status  = S_OK;

    const char  *EntryPoint   = "VSMain";
    const char  *Profile      = "vs_5_0";
    const char  *ByteCode     = CimDx11_VertexShader;
    const SIZE_T ByteCodeSize = strlen(CimDx11_VertexShader);

    UINT CompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

    *OutShaderBlob = CimDx11_CompileShader(ByteCode, ByteCodeSize,
                                           EntryPoint, Profile,
                                           Defines, CompileFlags);

    ID3D11VertexShader *VertexShader = NULL;
    Status = Backend->Device->CreateVertexShader((*OutShaderBlob)->GetBufferPointer(), (*OutShaderBlob)->GetBufferSize(), NULL, &VertexShader);
    Cim_AssertHR(Status);

    return VertexShader;
}

static ID3D11PixelShader *
CimDx11_CreatePxlShader(D3D_SHADER_MACRO *Defines)
{
    HRESULT           Status  = S_OK;
    ID3DBlob         *Blob    = NULL;
    cim_dx11_backend *Backend = (cim_dx11_backend *)CimCurrent->Backend;

    const char  *EntryPoint   = "PSMain";
    const char  *Profile      = "ps_5_0";
    const char  *ByteCode     = CimDx11_PixelShader;
    const SIZE_T ByteCodeSize = strlen(CimDx11_PixelShader);

    UINT CompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

    Blob = CimDx11_CompileShader(ByteCode, ByteCodeSize,
                                 EntryPoint, Profile,
                                 Defines, CompileFlags);

    ID3D11PixelShader *PixelShader = NULL;
    Status = Backend->Device->CreatePixelShader(Blob->GetBufferPointer(), Blob->GetBufferSize(), NULL, &PixelShader);
    Cim_AssertHR(Status);

    return PixelShader;
}

static UINT
CimDx11_GetFormatSize(DXGI_FORMAT Format)
{
    switch (Format)
    {

    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
        return 4;

    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        return 8;

    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_TYPELESS:
        return 12;

    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        return 16;

    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_D16_UNORM:
        return 2;

    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16_SINT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_TYPELESS:
        return 4;

    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        return 8;

    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_TYPELESS:
        return 1;

    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SINT:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
        return 2;

    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_R10G10B10A2_UINT:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R11G11B10_FLOAT:
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
        return 4;

    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
    case DXGI_FORMAT_B5G6R5_UNORM:
    case DXGI_FORMAT_B5G5R5A1_UNORM:
        return 2;

    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        return 4;

    default:
        return 0;

    }
}

static ID3D11InputLayout *
CimDx11_CreateInputLayout(ID3DBlob *VtxBlob, UINT *OutStride)
{
    ID3D11ShaderReflection *Reflection = NULL;
    D3DReflect(VtxBlob->GetBufferPointer(), VtxBlob->GetBufferSize(), IID_ID3D11ShaderReflection, (void **)&Reflection);

    D3D11_SHADER_DESC ShaderDesc;
    Reflection->GetDesc(&ShaderDesc);

    D3D11_INPUT_ELEMENT_DESC Desc[32] = {};
    UINT                     Offset = 0;
    for (cim_u32 InputIdx = 0; InputIdx < ShaderDesc.InputParameters; InputIdx++)
    {
        D3D11_SIGNATURE_PARAMETER_DESC Signature;
        Reflection->GetInputParameterDesc(InputIdx, &Signature);

        D3D11_INPUT_ELEMENT_DESC *InputDesc = Desc + InputIdx;
        InputDesc->SemanticName   = Signature.SemanticName;
        InputDesc->SemanticIndex  = Signature.SemanticIndex;
        InputDesc->InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

        if (Signature.Mask == 1)
        {
            if      (Signature.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                InputDesc->Format = DXGI_FORMAT_R32_UINT;
            else if (Signature.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                InputDesc->Format = DXGI_FORMAT_R32_SINT;
            else if (Signature.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                InputDesc->Format = DXGI_FORMAT_R32_FLOAT;
        }
        else if (Signature.Mask <= 3)
        {
            if      (Signature.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                InputDesc->Format = DXGI_FORMAT_R32G32_UINT;
            else if (Signature.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                InputDesc->Format = DXGI_FORMAT_R32G32_SINT;
            else if (Signature.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                InputDesc->Format = DXGI_FORMAT_R32G32_FLOAT;
        }
        else if (Signature.Mask <= 7)
        {
            if      (Signature.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                InputDesc->Format = DXGI_FORMAT_R32G32B32_UINT;
            else if (Signature.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                InputDesc->Format = DXGI_FORMAT_R32G32B32_SINT;
            else if (Signature.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                InputDesc->Format = DXGI_FORMAT_R32G32B32_FLOAT;
        }
        else if (Signature.Mask <= 15)
        {
            if      (Signature.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                InputDesc->Format = DXGI_FORMAT_R32G32B32A32_UINT;
            else if (Signature.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                InputDesc->Format = DXGI_FORMAT_R32G32B32A32_SINT;
            else if (Signature.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                InputDesc->Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        }

        InputDesc->AlignedByteOffset = Offset;
        Offset += CimDx11_GetFormatSize(InputDesc->Format);
    }

    HRESULT            Status = S_OK;
    ID3D11InputLayout *Layout = NULL;
    cim_dx11_backend *Backend = (cim_dx11_backend *)CimCurrent->Backend;

    Status = Backend->Device->CreateInputLayout(Desc, ShaderDesc.InputParameters,
                                                VtxBlob->GetBufferPointer(), VtxBlob->GetBufferSize(),
                                                &Layout);
    Cim_AssertHR(Status);

    *OutStride = Offset;

    return Layout;
}

static void 
CimDx11_Initialize(ID3D11Device *UserDevice, ID3D11DeviceContext *UserContext)
{
    if (!CimCurrent)
    {
        Cim_Assert(!"Context must be initialized.");
        return;
    }

    cim_dx11_backend *Backend = (cim_dx11_backend *)malloc(sizeof(cim_dx11_backend));

    if (!Backend)
    {
        return;
    }
    else
    {
        memset(Backend, 0, sizeof(cim_dx11_backend));
    }

    Backend->Device        = UserDevice;
    Backend->DeviceContext = UserContext;

    CimCurrent->Backend = Backend;
}

static cim_dx11_pipeline
CimDx11_CreatePipeline(cim_bit_field Features)
{
    cim_dx11_pipeline Pipeline = {};

    cim_u32          Enabled = 0;
    D3D_SHADER_MACRO Defines[CimFeature_Count + 1] = {};

    if (Features & CimFeature_AlbedoMap)
    {
        Defines[Enabled++] = (D3D_SHADER_MACRO){ "HAS_ALBEDO_MAP", "1" };
    }

    if (Features & CimFeature_MetallicMap)
    {
        Defines[Enabled++] = (D3D_SHADER_MACRO){ "HAS_METALLIC_MAP", "1" };
    }

    ID3DBlob *VSBlob = NULL;
    Pipeline.VtxShader = CimDx11_CreateVtxShader(Defines, &VSBlob); Cim_Assert(VSBlob);
    Pipeline.PxlShader = CimDx11_CreatePxlShader(Defines);
    Pipeline.Layout    = CimDx11_CreateInputLayout(VSBlob, &Pipeline.Stride);

    CimDx11_Release(VSBlob);

    return Pipeline;
}

static cim_dx11_pipeline *
CimDx11_GetOrCreatePipeline(cim_bit_field Key, cim_dx11_pipeline_hashmap *Hashmap)
{
    if (!Hashmap->IsInitialized)
    {
        Hashmap->GroupCount = 32;

        cim_u32 BucketCount = Hashmap->GroupCount * CimBucketGroupSize;

        Hashmap->Buckets  = (cim_dx11_entry *)malloc(BucketCount * sizeof(cim_dx11_entry));
        Hashmap->Metadata = (cim_u8 *)        malloc(BucketCount * sizeof(cim_u8));

        if (!Hashmap->Buckets || !Hashmap->Metadata)
        {
            return NULL;
        }

        memset(Hashmap->Buckets, 0, BucketCount * sizeof(cim_dx11_entry));
        memset(Hashmap->Metadata, CimEmptyBucketTag, BucketCount * sizeof(cim_u8));

        Hashmap->IsInitialized = true;
    }

    cim_u32 ProbeCount  = 0;
    cim_u32 HashedValue = CimHash_Block32(&Key, sizeof(Key));
    cim_u32 GroupIndex  = HashedValue & (Hashmap->GroupCount - 1);

    while (true)
    {
        cim_u8 *Meta = Hashmap->Metadata + (GroupIndex * CimBucketGroupSize);
        cim_u8  Tag  = (HashedValue & 0x7F);

        __m128i MetaVector = _mm_loadu_si128((__m128i *)Meta);
        __m128i TagVector  = _mm_set1_epi8(Tag);

        cim_i32 Mask = _mm_movemask_epi8(_mm_cmpeq_epi8(MetaVector, TagVector));

        while (Mask)
        {
            cim_u32 Lane  = CimHash_FindFirstBit32(Mask);
            cim_u32 Index = (GroupIndex * CimBucketGroupSize) + Lane;

            cim_dx11_entry *Entry = Hashmap->Buckets + Index;
            if (Entry->Key == Key)
            {
                return &Entry->Value;
            }

            Mask &= Mask - 1;
        }

        __m128i EmptyVector = _mm_set1_epi8(CimEmptyBucketTag);
        cim_i32 MaskEmpty = _mm_movemask_epi8(_mm_cmpeq_epi8(MetaVector, EmptyVector));

        if (MaskEmpty)
        {
            cim_u32 Lane = CimHash_FindFirstBit32(MaskEmpty);
            cim_u32 Index = (GroupIndex * CimBucketGroupSize) + Lane;

            cim_dx11_entry *Entry = Hashmap->Buckets + Index;
            Entry->Key = Key;
            Entry->Value = CimDx11_CreatePipeline(Key);

            Meta[Lane] = Tag;

            return &Entry->Value;
        }

        ProbeCount++;
        GroupIndex = (GroupIndex + (ProbeCount * ProbeCount)) & (Hashmap->GroupCount - 1);
    }
}

static void
CimDx11_SetupRenderState(cim_i32 ClientWidth, cim_i32 ClientHeight, cim_dx11_backend *Backend)
{
    ID3D11Device        *Device    = Backend->Device;
    ID3D11DeviceContext *DeviceCtx = Backend->DeviceContext;

    if (!Backend->SharedFrameData)
    {
        D3D11_BUFFER_DESC Desc = {};
        Desc.ByteWidth = sizeof(cim_dx11_shared_data);
        Desc.Usage = D3D11_USAGE_DYNAMIC;
        Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        HRESULT Status = Device->CreateBuffer(&Desc, NULL, &Backend->SharedFrameData);
        Cim_AssertHR(Status);
    }

    D3D11_MAPPED_SUBRESOURCE MappedResource;
    if (DeviceCtx->Map(Backend->SharedFrameData, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource) == S_OK)
    {
        cim_dx11_shared_data *SharedData = (cim_dx11_shared_data *)MappedResource.pData;

        cim_f32 Left = 0;
        cim_f32 Right = ClientWidth;
        cim_f32 Top = 0;
        cim_f32 Bot = ClientHeight;

        cim_f32 SpaceMatrix[4][4] =
        {
            { 2.0f / (Right - Left)          , 0.0f                     , 0.0f, 0.0f },
            { 0.0f                           , 2.0f / (Top - Bot)       , 0.0f, 0.0f },
            { 0.0f                           , 0.0f                     , 0.5f, 0.0f },
            { (Right + Left) / (Left - Right), (Top + Bot) / (Bot - Top), 0.5f, 1.0f },
        };

        memcpy(&SharedData->SpaceMatrix, SpaceMatrix, sizeof(SpaceMatrix));
        DeviceCtx->Unmap(Backend->SharedFrameData, 0);
    }

    DeviceCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    DeviceCtx->IASetIndexBuffer(Backend->IdxBuffer, DXGI_FORMAT_R32_UINT, 0);
    DeviceCtx->VSSetConstantBuffers(0, 1, &Backend->SharedFrameData);

    D3D11_VIEWPORT Viewport;
    Viewport.TopLeftX = 0.0f;
    Viewport.TopLeftY = 0.0f;
    Viewport.Width    = ClientWidth;
    Viewport.Height   = ClientHeight;
    Viewport.MinDepth = 0.0f;
    Viewport.MaxDepth = 1.0f;
    DeviceCtx->RSSetViewports(1, &Viewport);
}

static void
CimDx11_RenderUI(cim_i32 ClientWidth, cim_i32 ClientHeight)
{
    Cim_Assert(CimCurrent);
    cim_dx11_backend *Backend = (cim_dx11_backend *)CimCurrent->Backend; Cim_Assert(Backend);

    if (ClientWidth == 0 || ClientHeight == 0)
    {
        return;
    }

    HRESULT              Status    = S_OK;
    ID3D11Device        *Device    = Backend->Device;        Cim_Assert(Device);
    ID3D11DeviceContext *DeviceCtx = Backend->DeviceContext; Cim_Assert(DeviceCtx);
    cim_command_buffer  *CmdBuffer = UIP_COMMANDS;

    if (!Backend->VtxBuffer || CmdBuffer->FrameVtx.At > Backend->VtxBufferSize)
    {
        CimDx11_Release(Backend->VtxBuffer);

        Backend->VtxBufferSize = CmdBuffer->FrameVtx.At + 1024;

        D3D11_BUFFER_DESC Desc = {};
        Desc.ByteWidth         = Backend->VtxBufferSize;
        Desc.Usage             = D3D11_USAGE_DYNAMIC;
        Desc.BindFlags         = D3D11_BIND_VERTEX_BUFFER;
        Desc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;

        Status = Device->CreateBuffer(&Desc, NULL, &Backend->VtxBuffer); Cim_AssertHR(Status);
    }

    if (!Backend->IdxBuffer || CmdBuffer->FrameIdx.At > Backend->IdxBufferSize)
    {
        CimDx11_Release(Backend->IdxBuffer);

        Backend->IdxBufferSize = CmdBuffer->FrameIdx.At + 1024;

        D3D11_BUFFER_DESC Desc = {};
        Desc.ByteWidth         = Backend->IdxBufferSize;
        Desc.Usage             = D3D11_USAGE_DYNAMIC;
        Desc.BindFlags         = D3D11_BIND_INDEX_BUFFER;
        Desc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;

        Status = Device->CreateBuffer(&Desc, NULL, &Backend->IdxBuffer); Cim_AssertHR(Status);
    }

    D3D11_MAPPED_SUBRESOURCE VtxResource = {};
    Status = DeviceCtx->Map(Backend->VtxBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &VtxResource); Cim_AssertHR(Status);
    if (FAILED(Status) || !VtxResource.pData)
    {
        return;
    }
    memcpy(VtxResource.pData, CmdBuffer->FrameVtx.Memory, CmdBuffer->FrameVtx.At);
    DeviceCtx->Unmap(Backend->VtxBuffer, 0);

    D3D11_MAPPED_SUBRESOURCE IdxResource = {};
    Status = DeviceCtx->Map(Backend->IdxBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &IdxResource); Cim_AssertHR(Status);
    if (FAILED(Status) || !IdxResource.pData)
    {
        return;
    }
    memcpy(IdxResource.pData, CmdBuffer->FrameIdx.Memory, CmdBuffer->FrameIdx.At);
    DeviceCtx->Unmap(Backend->IdxBuffer, 0);

    // ===============================================================================

    // Do we just inline this?
    CimDx11_SetupRenderState(ClientWidth, ClientHeight, Backend);

    // ===============================================================================
    for (cim_u32 CmdIdx = 0; CmdIdx < CmdBuffer->CommandCount; CmdIdx++)
    {
        cim_draw_command  *Command  = CmdBuffer->Commands + CmdIdx;
        cim_dx11_pipeline *Pipeline = CimDx11_GetOrCreatePipeline(Command->Features, &Backend->PipelineStore);

        Cim_Assert(Command && Pipeline);

        DeviceCtx->IASetInputLayout(Pipeline->Layout);
        DeviceCtx->IASetVertexBuffers(0, 1, &Backend->VtxBuffer, &Pipeline->Stride, &Pipeline->Offset);

        DeviceCtx->VSSetShader(Pipeline->VtxShader, NULL, 0);

        DeviceCtx->PSSetShader(Pipeline->PxlShader, NULL, 0);

        DeviceCtx->DrawIndexed(Command->IdxCount, Command->IdxOffset, Command->VtxOffset);
    }

    CimArena_Reset(&CmdBuffer->FrameVtx);
    CimArena_Reset(&CmdBuffer->FrameIdx);
    CmdBuffer->CommandCount = 0;
}

#endif // _WIN32