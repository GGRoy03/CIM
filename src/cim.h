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
typedef uint32_t cim_u32;
typedef int      cim_i32;
typedef float    cim_f32;
typedef double   cim_f64;
typedef cim_u32  cim_bit_field;

#define Cim_Assert(Cond) do { if (!(Cond)) __debugbreak(); } while (0)
#define CIM_ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))

// [Private API] =============================

#define CimDefault_CmdStreamSize      32
#define CimDefault_CmdStreamGrowShift 1
#define CimIO_MaxPath 256

typedef enum QueryCimComponent_Flag
{
    QueryComponent_NoFlags        = 0,
    QueryComponent_IsTreeRoot     = 1 << 0,
    QueryComponent_AvoidHierarchy = 1 << 1,
} QueryCimComponent_Flag;

typedef enum CimLog_Severity
{
    CimLog_Info    = 0,
    CimLog_Warning = 1,
    CimLog_Error   = 2,
    CimLog_Fatal   = 3,
} CimLog_Severity;

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

#define CimIO_KeyboardKeyCount 256
#define CimIO_KeybordEventByufferSize 128

typedef enum CimMouse_Button
{
    CimMouse_Left = 0,
    CimMouse_Right = 1,
    CimMouse_Middle = 2,
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

// [Geometry:Types]

typedef enum CimShape_Type
{
    CimShape_Component = 0,
    CimShape_Rectangle = 1,
} CimShape_Type;

typedef struct cim_point
{
    CimShape_Type Shape;
    cim_u32       ScreenX; 
    cim_u32       ScreenY;
} cim_point;

typedef struct cim_geometry
{
    cim_point PackedPoints[32];
    cim_u32   Count;
    cim_u32   Size;
} cim_geometry;

// [Layout:Types]

typedef enum CimComponent_Flag
{
    CimComponent_Invalid = 0,
    CimComponent_Window  = 1 << 0,
    CimComponent_Button  = 1 << 1,
} CimComponent_Flag;

typedef struct cim_window_style
{
    cim_vector4 HeadColor; // Think colors can be compressed. (4bits/Col?)
    cim_vector4 BodyColor; // Think colors can be compressed. (4bits/Col?)

    cim_u32     BorderWidth;
    cim_vector4 BorderColor;
} cim_window_style;

typedef struct cim_window
{
    cim_window_style Style;
    
    // Geometry
    cim_point *Head;
    cim_point *Body;
    cim_point *Border;
} cim_window;

typedef struct cim_button_style
{
    cim_vector4 BodyColor;

    cim_u32     BorderWidth;
    cim_vector4 BorderColor;

    cim_vector2 Position;
    cim_vector2 Dimension;
} cim_button_style;

typedef struct cim_button
{
    cim_button_style Style;

    // Geometry: Do we really need the border?
    cim_point *Rect;
    cim_point *Border;
} cim_button;

typedef struct cim_component
{
    bool IsInitialized;

    CimComponent_Flag Type;
    union
    {
        cim_window Window;
        cim_button Button;
    } For;

    cim_bit_field StyleUpdateFlags;
} cim_component;

typedef struct layout_node
{
    struct
    layout_node **Children;
    cim_u32       ChildCount;
    cim_u32       ChildSize;
    cim_component Component;
} layout_node;

typedef struct cim_component_entry
{
    char         Key[64];
    layout_node *Node;    // WARN: This might be dangerous.
} cim_component_entry;

typedef struct cim_component_hashmap
{
    cim_u8              *Metadata;
    cim_component_entry *Buckets;
    cim_u32              GroupCount;
    bool                 IsInitialized;
} cim_component_hashmap;

typedef struct cim_layout_tree
{
    cim_component_hashmap ComponentMap; // Maps ID -> Node

    layout_node  Root;
    layout_node *Nodes;
    cim_u32      NodeCount;
    cim_u32      NodeSize;

    layout_node *AtNode;
} cim_layout_tree;

// [Styling:Types]

#define FAIL_ON_NEGATIVE(Negative, Message, ...) if(Negative) {CimLog_Error(Message, __VA_ARGS__);}
#define ARRAY_TO_VECTOR2(Array, Length, Vector)  if(Length > 0) Vector.x = Array[0]; if(Length > 1) Vector.y = Array[1];
#define ARRAY_TO_VECTOR4(Array, Length, Vector)  if(Length > 0) Vector.x = Array[0]; if(Length > 1) Vector.y = Array[1]; \
                                                 if(Length > 2) Vector.z = Array[2]; if(Length > 3) Vector.w = Array[3];
typedef enum StyleUpdate_Flag
{
    StyleUpdate_Unknown = 0,
    StyleUpdate_BorderGeometry = 1 << 0,
} StyleUpdate_Flag;

typedef enum Attribute_Type
{
    Attr_HeadColor   = 0,
    Attr_BodyColor   = 1,
    Attr_BorderColor = 2,
    Attr_BorderWidth = 3,
    Attr_Position    = 4,
    Attr_Dimension   = 5,
} Attribute_Type;

typedef enum Token_Type
{
    Token_String     = 255,
    Token_Identifier = 256,
    Token_Number     = 257,
    Token_Assignment = 258,
    Token_Vector     = 269,
} Token_Type;

// NOTE: Is this used? If not, why not?
typedef enum ValueFormat_Flag
{
    ValueFormat_SNumber = 1 << 0,
    ValueFormat_UNumber = 1 << 1,
    ValueFormat_Vector  = 1 << 2,
} ValueFormat_Flag;

typedef struct master_style
{
    cim_vector4 HeadColor;
    cim_vector4 BodyColor;

    cim_vector4 BorderColor;
    cim_u32     BorderWidth;
    bool        HasBorders;

    cim_vector2 Position;
    cim_vector2 Dimension;
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
    const char *Name;
    size_t         Length;
    CimComponent_Flag ComponentFlag;
} valid_component;

typedef struct valid_attribute
{
    const char *Name;
    size_t           Length;
    Attribute_Type   Type;
    ValueFormat_Flag FormatFlags;
    cim_bit_field    ComponentFlag;
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
    // Raw data to be uploaded by the backend.
    cim_arena FrameVtx;
    cim_arena FrameIdx;

    // Instead of batches, we use commands.
    cim_draw_command *Commands;
    cim_u32           CommandCount;
    cim_u32           CommandSize;

    // Flags to reset batches.
    bool ClippingRectChanged;
    bool FeatureStateChanged;
} cim_command_buffer;

// [Global Context]

typedef struct cim_context
{
    cim_layout_tree    Layout;
    cim_inputs         Inputs;
    cim_geometry       Geometry;
    cim_command_buffer Commands;
    void              *Backend;
} cim_context;

static cim_context *CimCurrent;

#define UI_LAYOUT       (CimCurrent->Layout)
#define UIP_LAYOUT     &(CimCurrent->Layout)
#define UI_INPUT        (CimCurrent->Inputs)
#define UIP_INPUT      &(CimCurrent->Inputs)
#define UI_GEOMETRY     (CimCurrent->Geometry)
#define UIP_GEOMETRY   &(CimCurrent->Geometry)
#define UI_COMMANDS     (CimCurrent->Commands)
#define UIP_COMMANDS   &(CimCurrent->Commands)

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
CimInput_GetMouseDeltaX(cim_inputs *Inputs)
{
    cim_i32 DeltaX = Inputs->MouseDeltaX;

    return DeltaX;
}

cim_i32
CimInput_GetMouseDeltaY(cim_inputs *Inputs)
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

// NOTE: Isn't this name quite confusing? It probably also has some performance
// implication, because we are accessing that pointer... Idk.

cim_rect 
MakeRectFromPoint(cim_point *Point)
{
    cim_rect Rect;

    Rect.MinX = Point[0].ScreenX;
    Rect.MinY = Point[0].ScreenY;
    Rect.MaxX = Point[1].ScreenX;
    Rect.MaxY = Point[1].ScreenY;

    return Rect;
}

bool
IsInsideRect(cim_rect Rect)
{
    cim_vector2 MousePos = GetMousePosition(UIP_INPUT);

    bool MouseIsInside = (MousePos.x > Rect.MinX) && (MousePos.x < Rect.MaxX) &&
                         (MousePos.y > Rect.MinY) && (MousePos.y < Rect.MaxY);

    return MouseIsInside;
}

// [Primitives]

// BUG: Does not check for overflows when allocating.

cim_point *
AllocShape(cim_vector2 At, cim_f32 Dim1, cim_f32 Dim2, CimShape_Type Type)
{
    Cim_Assert(CimCurrent);
    cim_geometry *Geometry = UIP_GEOMETRY;

    switch (Type)
    {

    case CimShape_Rectangle:
    {
        // Dim1 = Width, Dim2 = Height

        cim_point *P0 = Geometry->PackedPoints + Geometry->Count++;
        P0->ScreenX = At.x;
        P0->ScreenY = At.y;
        P0->Shape = CimShape_Rectangle;

        cim_point *P1 = Geometry->PackedPoints + Geometry->Count++;
        P1->ScreenX = At.x + Dim1;
        P1->ScreenY = At.y + Dim2;

        return P0;
    }

    default:
    {
        CimLog_Fatal("Impossible to allocate shape with unknown type.");
        return NULL;
    }

    }
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

// BUG: Can overflow when reading Buffer->Commands + Buffer->CommandCount.
//      Also, the Buffer->Commands is never allocated. Where do we write
//      that logic?

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
DrawQuad(cim_point *Point, cim_vector4 Col)
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

    // WARN: Doing point[1] might bug prone?
    cim_f32 x0 = Point[0].ScreenX;
    cim_f32 y0 = Point[0].ScreenY;
    cim_f32 x1 = Point[1].ScreenX;
    cim_f32 y1 = Point[1].ScreenY;

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

layout_node *
AllocateNode(cim_layout_tree *Tree)
{
    if (!Tree)
    {
        CimLog_Fatal("Internal cim_layout_tree: Tree does not exist.");
        return NULL;
    }

    if (Tree->NodeCount == Tree->NodeSize)
    {
        Tree->NodeSize = Tree->NodeSize ? Tree->NodeSize * 2 : 8;

        layout_node *New = (layout_node*)malloc(Tree->NodeSize * sizeof(layout_node));
        if (!New)
        {
            CimLog_Fatal("Internal cim_layout_tree: Malloc failure (OOM?)");
            return NULL;
        }

        if (Tree->Nodes)
        {
            memcpy(New, Tree->Nodes, Tree->NodeCount * sizeof(layout_node));
            free(Tree->Nodes);
        }

        Tree->Nodes = New;
    }

    layout_node *Node = Tree->Nodes + Tree->NodeCount++;
    memset(Node, 0, sizeof(layout_node));

    return Node;
}

cim_component_entry *
FindComponent(const char *Key)
{
    Cim_Assert(CimCurrent);
    cim_component_hashmap *Hashmap = &UI_LAYOUT.ComponentMap;

    if (!Hashmap->IsInitialized)
    {
        Hashmap->GroupCount = 32;

        cim_u32 BucketCount = Hashmap->GroupCount * CimBucketGroupSize;
        size_t  BucketSize = BucketCount * sizeof(cim_component_entry);
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
                return E;
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

            return E;
        }

        ProbeCount++;
        GroupIdx = (GroupIdx + ProbeCount * ProbeCount) & (Hashmap->GroupCount - 1);
    }

    return NULL;
}

// WARN: This code is still really bad. Need to figure out
// what we want to do for the layout. Also really buggy.

cim_component *
QueryComponent(const char *Key, cim_bit_field Flags)
{
    Cim_Assert(CimCurrent && "Forgot to initialize a context?");
    cim_layout_tree *Tree = UIP_LAYOUT;

    cim_component_entry *Entry = FindComponent(Key);
    if (!Entry->Node && !(Flags & QueryComponent_IsTreeRoot))
    {
        Entry->Node = AllocateNode(Tree);
    }
    else if (!Entry->Node && Flags & QueryComponent_IsTreeRoot)
    {
        Entry->Node = &Tree->Root; // Set the node equal to our only tree.
    }

    if (!(Flags & QueryComponent_AvoidHierarchy))
    {
        if (Flags & QueryComponent_IsTreeRoot)
        {
            // Not sure.
        }
        else
        {
            layout_node *Node = Tree->AtNode;
            if (Node->ChildCount == Node->ChildSize)
            {
                Node->ChildSize = Node->ChildSize ? Node->ChildSize * 2 : 4;

                layout_node **New = (layout_node**)malloc(Node->ChildSize * sizeof(layout_node *));
                if (!New)
                {
                    CimLog_Fatal("Internal cim_layout_tree: Malloc failure (OOM?)");
                    return NULL;
                }

                if (Node->Children)
                {
                    memcpy(New, Node->Children, Node->ChildCount * sizeof(layout_node));
                    free(Node->Children);
                }

                Node->Children = New;
            }

            Node->Children[Node->ChildCount] = Entry->Node;
            Tree->AtNode = Entry->Node;
        }
    }

    Tree->AtNode = Entry->Node;

    return &Entry->Node->Component;
}

// [Constraints]

typedef struct cim_draggable
{
    cim_point *Points[4];
    cim_u32    Count;
} cim_draggable;

void
ApplyDraggable(cim_draggable *Registered, cim_u32 RegisteredCount, cim_i32 MouseDeltaX, cim_i32 MouseDeltaY)
{
    for (cim_u32 DragIdx = 0; DragIdx < RegisteredCount; DragIdx++)
    {
        cim_draggable *Draggable = Registered + DragIdx;
        cim_point     *P0        = Draggable->Points[0];

        switch (P0->Shape)
        {

        case CimShape_Rectangle:
        {
            cim_point *P1 = P0 + 1;

            cim_point TopLeft     = *P0;
            cim_point BottomRight = *P1;

            cim_rect HitBox;
            HitBox.MinX = TopLeft.ScreenX;
            HitBox.MinY = TopLeft.ScreenY;
            HitBox.MaxX = BottomRight.ScreenX;
            HitBox.MaxY = BottomRight.ScreenY;

            if (IsInsideRect(HitBox))
            {
                P0->ScreenX += MouseDeltaX;
                P0->ScreenY += MouseDeltaY;

                P1->ScreenX += MouseDeltaX;
                P1->ScreenY += MouseDeltaY;
            }
        } break;

        default:
            CimLog_Error("Invalid shape handled when apllying drag.");
            return;

        }
    }

}

void
SolveUIConstraints(cim_inputs *Inputs)
{
    bool MouseDown = IsMouseDown(CimMouse_Left, Inputs);

    cim_i32 MDeltaX = CimInput_GetMouseDeltaX(Inputs);
    cim_i32 MDeltaY = CimInput_GetMouseDeltaY(Inputs);

    if (MouseDown)
    {
        ApplyDraggable(NULL, 0, MDeltaX, MDeltaY);
    }
}

// [Styles]

valid_component ValidComponents[] =
{
    {"Window", (sizeof("Window") - 1), CimComponent_Window},
    {"Button", (sizeof("Button") - 1), CimComponent_Button},
};

valid_attribute ValidAttributes[] =
{
    {"BodyColor", (sizeof("BodyColor") - 1), Attr_BodyColor,
    ValueFormat_Vector, CimComponent_Window | CimComponent_Button},

    {"HeadColor", (sizeof("HeadColor") - 1), Attr_HeadColor,
    ValueFormat_Vector, CimComponent_Window},

    {"BorderColor", (sizeof("BorderColor") - 1), Attr_BorderColor,
    ValueFormat_Vector, CimComponent_Window | CimComponent_Button},

    {"BorderWidth", (sizeof("BorderWidth") - 1), Attr_BorderWidth,
    ValueFormat_UNumber, CimComponent_Window | CimComponent_Button},

    {"Position", (sizeof("Position") - 1), Attr_Position,
    ValueFormat_Vector, CimComponent_Window | CimComponent_Button},

    {"Dimension", (sizeof("Dimension") - 1), Attr_Dimension,
    ValueFormat_Vector, CimComponent_Window | CimComponent_Button},
};

// TODO: Possibly move this out of here?

buffer
ReadEntireFile(const char *File)
{
    buffer Result = { 0 };

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
    lexer Lexer = { 0 };
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
            // NOTE: The formatting rules are quite strict.

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

                    if (C >= '0' && C <= '9') Digit = C - '0';
                    else if (C >= 'A' && C <= 'F') Digit = C - 'A' + 10;
                    else if (C >= 'a' && C <= 'f') Digit = C - 'a' + 10;
                    else { Valid = false; break; }

                    Value = (Value << 4) | Digit;
                }

                if (!Valid) break;

                Vector[VectorIdx++] = Value * Inverse;
                HexCount += 2;
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
    user_styles Styles = { 0 };
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
            valid_attribute Attr = { 0 };
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

            switch (Attr.Type)
            {

            case Attr_BodyColor:
            {
                FAIL_ON_NEGATIVE(IsNegative, "Value cannot be negative.");
                ARRAY_TO_VECTOR4(Next->Vector, Next->VectorSize, Desc->Style.BodyColor)
            } break;

            case Attr_HeadColor:
            {
                FAIL_ON_NEGATIVE(IsNegative, "Value cannot be negative.");
                ARRAY_TO_VECTOR4(Next->Vector, Next->VectorSize, Desc->Style.HeadColor)
            } break;

            case Attr_BorderColor:
            {
                FAIL_ON_NEGATIVE(IsNegative, "Value cannot be negative.");
                ARRAY_TO_VECTOR4(Next->Vector, Next->VectorSize, Desc->Style.BorderColor)
            } break;

            case Attr_BorderWidth:
            {
                FAIL_ON_NEGATIVE(IsNegative, "Value cannot be negative.");
                Desc->Style.BorderWidth = Next->UInt32;
            } break;

            case Attr_Position:
            {
                FAIL_ON_NEGATIVE(IsNegative, "Value cannot be negative");
                ARRAY_TO_VECTOR2(Next->Vector, Next->VectorSize, Desc->Style.Position)
            } break;

            case Attr_Dimension:
            {
                FAIL_ON_NEGATIVE(IsNegative, "Value cannot be negative");
                ARRAY_TO_VECTOR2(Next->Vector, Next->VectorSize, Desc->Style.Dimension)
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
        style_desc *Desc = Styles->Descs + DescIdx;

        cim_bit_field Flags = QueryComponent_AvoidHierarchy;
        Flags |= Desc->ComponentFlag & CimComponent_Window ? QueryComponent_IsTreeRoot : 0;

        cim_component *Component = QueryComponent(Desc->Id, Flags);

        switch (Desc->ComponentFlag)
        {

        case CimComponent_Window:
        {
            cim_window *Window = &Component->For.Window;

            Window->Style.BodyColor = Desc->Style.BodyColor;
            Window->Style.HeadColor = Desc->Style.HeadColor;

            Window->Style.BorderColor = Desc->Style.BorderColor;
            Window->Style.BorderWidth = Desc->Style.BorderWidth;

            // WARN: Force set this for now. When we rewrite the hot-reload
            // we need to fix this.
            Component->StyleUpdateFlags |= StyleUpdate_BorderGeometry;
        } break;

        case CimComponent_Button:
        {
            cim_button *Button = &Component->For.Button;

            Button->Style.BodyColor = Desc->Style.BodyColor;

            Button->Style.Position = Desc->Style.Position;
            Button->Style.Dimension = Desc->Style.Dimension;
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
    buffer      FileContent = { 0 };
    lexer       Lexer = { 0 };
    user_styles Styles = { 0 };

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


// [Public API] =============================

typedef enum CimWindow_Flags
{
    CimWindow_Draggable = 1 << 0,
} CimWindow_Flags;

bool
Cim_Window(const char *Id, cim_bit_field Flags)
{
    cim_component   *Component = QueryComponent(Id, QueryComponent_IsTreeRoot);
    cim_window      *Window    = &Component->For.Window;
    cim_window_style Style     = Window->Style;

    UI_COMMANDS.ClippingRectChanged = true; // Uhm..

    if (Component->Type == CimComponent_Invalid)
    {
        cim_vector2 HeadAt = { 500.0f, 500.0f };
        cim_f32     Width  = 300.0f;
        cim_f32     Height = 150.0f;
        Window->Head = AllocShape(HeadAt, Width, Height, CimShape_Rectangle);

        cim_vector2 BodyAt  = { 500.0f, 650.0f };
        cim_f32     Width2  = 300.0f;
        cim_f32     Height2 = 400.0f;
        Window->Body = AllocShape(BodyAt, Width2, Height2, CimShape_Rectangle);

        // NOTE: These memory accesses are kind of weird still.
        cim_f32 TopLeftX = Window->Head->ScreenX - Style.BorderWidth;
        cim_f32 TopLeftY = Window->Head->ScreenY - Style.BorderWidth;
        cim_u32 BWidth   = (Window->Body[1].ScreenX - Window->Head[0].ScreenX) + (2 * Style.BorderWidth);
        cim_u32 BHeight  = (Window->Body[1].ScreenY - Window->Head[0].ScreenY) + (2 * Style.BorderWidth);
        Window->Border = AllocShape({TopLeftX, TopLeftY}, BWidth, BHeight, CimShape_Rectangle);

        Component->Type = CimComponent_Window;
    }

    if (Window->Style.BorderWidth > 0)
    {
        DrawQuad(Window->Border, Window->Style.BorderColor);
    }
    DrawQuad(Window->Head, Window->Style.HeadColor);
    DrawQuad(Window->Body, Window->Style.BodyColor);

    return true;
}

bool
Cim_Button(const char *Id)
{
    cim_component    *Component = QueryComponent(Id, QueryComponent_NoFlags);
    cim_button       *Button    = &Component->For.Button;
    cim_button_style  Style     = Button->Style;

    if (Component->Type == CimComponent_Invalid)
    {
        Button->Rect = AllocShape(Style.Position, Style.Dimension.x, Style.Dimension.y, CimShape_Rectangle);

        Component->Type = CimComponent_Button;
    }

    DrawQuad(Button->Rect, Style.BodyColor);

    cim_rect HitBox    = MakeRectFromPoint(Button->Rect);
    bool     IsClicked = IsInsideRect(HitBox) && IsMouseClicked(CimMouse_Left, UIP_INPUT);

    return IsClicked;
}

void
Cim_EndFrame()
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

#define CimFeature_Count 2

typedef enum CimFeature_Type
{
    CimFeature_AlbedoMap = 1 << 0,
    CimFeature_MetallicMap = 1 << 1,
} CimFeature_Type;

// [Platforms]

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <windowsx.h>

void
CimWin32_LogMessage(CimLog_Severity Level, const char *File, cim_i32 Line, const char *Format, va_list Args)
{
    char Buffer[1024] = { 0 };
    vsnprintf(Buffer, sizeof(Buffer), Format, Args);

    char FinalMessage[2048] = { 0 };
    snprintf(FinalMessage, sizeof(FinalMessage), "[%s:%d] %s\n", File, Line, Buffer);

    OutputDebugStringA(FinalMessage);
}

void
CimWin32_ProcessInputMessage(cim_button_state *NewState, bool IsDown)
{
    if (NewState->EndedDown != IsDown)
    {
        NewState->EndedDown = IsDown;
        ++NewState->HalfTransitionCount;
    }
}

DWORD WINAPI
IOThreadProc(LPVOID Param)
{
    cim_file_watcher_context *WatchContext = (cim_file_watcher_context *)Param;

    HANDLE DirHandle = CreateFileA(WatchContext->Directory, FILE_LIST_DIRECTORY,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                   NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (DirHandle == INVALID_HANDLE_VALUE)
    {
        CimLog_Fatal("Win32 IO Thread: Failed to open directory provided by the user: %s", WatchContext->Directory);
        return 1;
    }

    BYTE  Buffer[4096];
    DWORD BytesReturned = 0;

    while (true)
    {
        DWORD Filter = FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_CREATION;
        BOOL  Okay = ReadDirectoryChangesW(DirHandle, Buffer, sizeof(Buffer),
            FALSE, Filter, &BytesReturned,
            NULL, NULL);
        if (!Okay)
        {
            CimLog_Error("ReadDirectoryChangesW failed: %u\n", GetLastError());
            break;
        }

        BYTE *Ptr = Buffer;
        do
        {
            FILE_NOTIFY_INFORMATION *Info = (FILE_NOTIFY_INFORMATION *)Ptr;

            char    Name[CimIO_MaxPath];
            cim_i32 Length = WideCharToMultiByte(CP_UTF8, 0,
                                                 Info->FileName, Info->FileNameLength / sizeof(WCHAR),
                                                 Name, sizeof(Name) - 1, NULL, NULL);
            Name[Length] = '\0';

            for (cim_u32 FileIdx = 0; FileIdx < WatchContext->FileCount; FileIdx++)
            {
                if (_stricmp(Name, WatchContext->Files[FileIdx].FileName) == 0)
                {
                    CimLog_Info("File has changed : %s", WatchContext->Files[FileIdx].FullPath);
                    CimStyle_Initialize(WatchContext->Files[FileIdx].FullPath);
                    break;
                }
            }

            if (Info->NextEntryOffset == 0)
            {
                break;
            }

            Ptr += Info->NextEntryOffset;
        } while (true);

    }

    CloseHandle(DirHandle);
    free(WatchContext->Files);
    free(WatchContext);

    return 0;
}

// TODO: Make it so we can specify a backend and this initializes it, also need a void *for args.

bool
CimWin32_Initialize(const char *StyleDirectory)
{
    // Initialize the IO Thread (The thread handles the resource cleanups)

    cim_file_watcher_context *IOContext = (cim_file_watcher_context*)calloc(1, sizeof(cim_file_watcher_context));
    if (!IOContext)
    {
        CimLog_Error("Win32 Init: Failed to allocate memory for the IO context");
        return false;
    }
    strncpy_s(IOContext->Directory, sizeof(IOContext->Directory), StyleDirectory, strlen(StyleDirectory)); // NOTE: Get rid of MSVC?
    IOContext->Directory[(sizeof(IOContext->Directory) - 1)] = '\0';

    WIN32_FIND_DATAA FindData;
    HANDLE           FindHandle = INVALID_HANDLE_VALUE;

    char   SearchPattern[MAX_PATH];
    size_t DirLength = strlen(StyleDirectory);

    // Path/To/Dir[/*.cim]" 6 Char
    if (DirLength + 6 >= MAX_PATH)
    {
        CimLog_Error("Win32 Init: Provided directory is too long: %s", StyleDirectory);
        return false;
    }
    snprintf(SearchPattern, MAX_PATH, "%s/*.cim", StyleDirectory);

    FindHandle = FindFirstFileA(SearchPattern, &FindData);
    if (FindHandle == INVALID_HANDLE_VALUE)
    {
        DWORD Error = GetLastError();
        if (Error == ERROR_FILE_NOT_FOUND)
        {
            CimLog_Warn("Win32 Init: Provided directory: %s does not contain any .cim files", StyleDirectory);
            return true;
        }
        else
        {
            CimLog_Error("Win32 Init: FindFirstFileA Failed with error %u", Error);
            return false;
        }
    }

    const cim_u32 Capacity = 4;

    IOContext->Files     = (cim_watched_file*)calloc(Capacity, sizeof(cim_watched_file));
    IOContext->FileCount = 0;
    if (!IOContext->Files)
    {
        CimLog_Error("Win32 Init: Failed to allocate memory for the wathched files.");
        return false;
    }

    do
    {
        if (!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            if (IOContext->FileCount == Capacity)
            {
                CimLog_Warn("Win32 Init: Maximum number of style files reached.");
                break;
            }

            cim_watched_file *Watched = IOContext->Files + IOContext->FileCount;

            size_t NameLength = strlen(FindData.cFileName);
            if (NameLength >= MAX_PATH)
            {
                CimLog_Error("Win32 Init: File name is too long: %s", FindData.cFileName);
                return false;
            }

            memcpy(Watched->FileName, FindData.cFileName, NameLength);
            Watched->FileName[NameLength] = '\0';

            size_t FullLength = DirLength + 1 + NameLength + 1;
            if (FullLength >= MAX_PATH)
            {
                CimLog_Error("Win32 Init: Full path for %s is too long.", FindData.cFileName);
                return false;
            }

            snprintf(Watched->FullPath, FullLength, "%s/%s", StyleDirectory, FindData.cFileName);
            Watched->FullPath[FullLength] = '\0';

            ++IOContext->FileCount;
        }
    } while (FindNextFileA(FindHandle, &FindData));
    FindClose(FindHandle);

    if (IOContext->FileCount > 0)
    {
        HANDLE IOThreadHandle = CreateThread(NULL, 0, IOThreadProc, IOContext, 0, NULL);
        if (!IOThreadHandle)
        {
            CimLog_Error("Win32 Init: Failed to launch IO Thread with error : %u", GetLastError());
            return false;
        }

        CloseHandle(IOThreadHandle);
    }

    CimLogger = CimWin32_LogMessage;

    // Call the style parser. Should provide the list of all of the sub-files
    // in the style directory. There could be some weird race error if the IO thread
    // exits early and frees the context. Right not just pass the first
    // one and don't worry about the race...
    CimStyle_Initialize(IOContext->Files[0].FullPath);

    return true;
}

LRESULT CALLBACK
CimWin32_WindowProc(HWND Handle, UINT Message, WPARAM WParam, LPARAM LParam)
{
    if (!CimCurrent)
    {
        return FALSE;
    }
    cim_inputs *Inputs = UIP_INPUT;

    switch (Message)
    {

    case WM_MOUSEMOVE:
    {
        cim_i32 MouseX = GET_X_LPARAM(LParam);
        cim_i32 MouseY = GET_Y_LPARAM(LParam);

        Inputs->MouseDeltaX += (MouseX - Inputs->MouseX);
        Inputs->MouseDeltaY += (MouseY - Inputs->MouseY);

        Inputs->MouseX = MouseX;
        Inputs->MouseY = MouseY;
    } break;

    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    {
        cim_u32 VKCode = (cim_u32)WParam;
        bool    WasDown = ((LParam & ((size_t)1 << 30)) != 0);
        bool    IsDown = ((LParam & ((size_t)1 << 31)) == 0);

        if (WasDown != IsDown && VKCode < CimIO_KeyboardKeyCount)
        {
            CimWin32_ProcessInputMessage(&Inputs->Buttons[VKCode], IsDown);
        }
    } break;

    case WM_LBUTTONDOWN:
    {
        CimWin32_ProcessInputMessage(&Inputs->MouseButtons[CimMouse_Left], true);
    } break;

    case WM_LBUTTONUP:
    {
        CimWin32_ProcessInputMessage(&Inputs->MouseButtons[CimMouse_Left], false);
    } break;

    case WM_RBUTTONDOWN:
    {
        CimWin32_ProcessInputMessage(&Inputs->MouseButtons[CimMouse_Right], true);
    } break;

    case WM_RBUTTONUP:
    {
        CimWin32_ProcessInputMessage(&Inputs->MouseButtons[CimMouse_Right], false);
    } break;

    case WM_MBUTTONDOWN:
    {
        CimWin32_ProcessInputMessage(&Inputs->MouseButtons[CimMouse_Middle], true);
    } break;

    case WM_MBUTTONUP:
    {
        CimWin32_ProcessInputMessage(&Inputs->MouseButtons[CimMouse_Middle], false);
    } break;

    case WM_MOUSEWHEEL:
    {
    } break;

    }

    return FALSE; // We don't want to block any messages right now.
}

#endif // _WIN32

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

ID3DBlob *
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

ID3D11VertexShader *
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

ID3D11PixelShader *
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

UINT
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

ID3D11InputLayout *
CimDx11_CreateInputLayout(cim_bit_field Features, ID3DBlob *VtxBlob, UINT *OutStride)
{
    ID3D11ShaderReflection *Reflection = NULL;
    D3DReflect(VtxBlob->GetBufferPointer(), VtxBlob->GetBufferSize(), IID_ID3D11ShaderReflection, (void **)&Reflection);

    D3D11_SHADER_DESC ShaderDesc;
    Reflection->GetDesc(&ShaderDesc);

    D3D11_INPUT_ELEMENT_DESC Desc[32] = { {0} };
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

// } -[SECTION:Shaders]

void CimDx11_Initialize(ID3D11Device *UserDevice, ID3D11DeviceContext *UserContext)
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

// [SECTION:Pipeline] {

cim_dx11_pipeline
CimDx11_CreatePipeline(cim_bit_field Features)
{
    cim_dx11_pipeline Pipeline = { 0 };

    cim_u32          Enabled = 0;
    D3D_SHADER_MACRO Defines[CimFeature_Count + 1] = { {0} };

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
    Pipeline.Layout = CimDx11_CreateInputLayout(Features, VSBlob, &Pipeline.Stride);

    CimDx11_Release(VSBlob);

    return Pipeline;
}

cim_dx11_pipeline *
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

// } [SECTION:Pipeline]

// [SECTION:Commands] {

// TODO:
// Implement the textures: Binding, Creation, Updating, Uber-Shader

void
CimDx11_SetupRenderState(cim_i32           ClientWidth,
    cim_i32           ClientHeight,
    cim_dx11_backend *Backend)
{
    ID3D11Device        *Device    = Backend->Device;
    ID3D11DeviceContext *DeviceCtx = Backend->DeviceContext;

    if (!Backend->SharedFrameData)
    {
        D3D11_BUFFER_DESC Desc = { 0 };
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

void
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

    // Who calls this? Still not set on when/how we want to process constraints.
    SolveUIConstraints(UIP_INPUT);

    if (!Backend->VtxBuffer || CmdBuffer->FrameVtx.At > Backend->VtxBufferSize)
    {
        CimDx11_Release(Backend->VtxBuffer);

        Backend->VtxBufferSize = CmdBuffer->FrameVtx.At + 1024;

        D3D11_BUFFER_DESC Desc = { 0 };
        Desc.ByteWidth = Backend->VtxBufferSize;
        Desc.Usage = D3D11_USAGE_DYNAMIC;
        Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        Status = Device->CreateBuffer(&Desc, NULL, &Backend->VtxBuffer); Cim_AssertHR(Status);
    }

    if (!Backend->IdxBuffer || CmdBuffer->FrameIdx.At > Backend->IdxBufferSize)
    {
        CimDx11_Release(Backend->IdxBuffer);

        Backend->IdxBufferSize = CmdBuffer->FrameIdx.At + 1024;

        D3D11_BUFFER_DESC Desc = { 0 };
        Desc.ByteWidth = Backend->IdxBufferSize;
        Desc.Usage = D3D11_USAGE_DYNAMIC;
        Desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        Status = Device->CreateBuffer(&Desc, NULL, &Backend->IdxBuffer); Cim_AssertHR(Status);
    }

    D3D11_MAPPED_SUBRESOURCE VtxResource = { 0 };
    Status = DeviceCtx->Map(Backend->VtxBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &VtxResource); Cim_AssertHR(Status);
    if (FAILED(Status) || !VtxResource.pData)
    {
        return;
    }
    memcpy(VtxResource.pData, CmdBuffer->FrameVtx.Memory, CmdBuffer->FrameVtx.At);
    DeviceCtx->Unmap(Backend->VtxBuffer, 0);

    D3D11_MAPPED_SUBRESOURCE IdxResource = { 0 };
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