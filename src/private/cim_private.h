#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <immintrin.h>

#ifdef __cplusplus
extern "C" {
#endif

// [MACROS & Type Alias]

#define Cim_Assert(Cond) do { if (!(Cond)) __debugbreak(); } while (0)
#define CIM_ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))

#define CIM_KEYBOARD_KEY_COUNT          256
#define CIM_KEYBOARD_EVENT_BUFFER_COUNT 128

#define CimDefault_CmdStreamSize      32
#define CimDefault_CmdStreamGrowShift 1

#define CimEmptyBucketTag  0x80
#define CimBucketGroupSize 16

#define CimIO_MaxPath 256

typedef uint8_t  cim_u8;
typedef uint32_t cim_u32;
typedef int      cim_i32;
typedef float    cim_f32;
typedef double   cim_f64;
typedef cim_u32  cim_bit_field;

// TODO: Move Vectors and rect out of here.
typedef struct cim_vector2
{
    cim_f32 x, y;
} cim_vector2;

typedef struct cim_vector4
{
    cim_f32 x, y, z, w;
} cim_vector4;

typedef struct cim_rect
{
    cim_vector2 Min;
    cim_vector2 Max;
} cim_rect;

// [STRUCTS]

typedef struct cim_point 
{
    cim_f32 x, y;
} cim_point;

typedef struct cim_point_node 
{
    cim_point Value;
    struct cim_point_node *Prev;
    struct cim_point_node *Next;
} cim_point_node;

typedef struct cim_primitive_rings 
{
    cim_point_node PointNodes[128];
    cim_u32        PointCount;
    cim_u32        PointNodesCapacity;
} cim_primitive_rings;

typedef struct cim_arena 
{
    void *Memory;
    size_t At;
    size_t Capacity;
} cim_arena;

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

typedef enum CimMouse_Button 
{
    CimMouse_Left,
    CimMouse_Right,
    CimMouse_Middle,
    CimMouse_ButtonCount,
} CimMouse_Button;

typedef struct cim_inputs 
{
    cim_button_state Buttons[CIM_KEYBOARD_KEY_COUNT];
    cim_f32          ScrollDelta;
    cim_i32          MouseX, MouseY;
    cim_i32          MouseDeltaX, MouseDeltaY;
    cim_button_state MouseButtons[5];
} cim_inputs;

typedef struct watched_file
{
    char FileName[CimIO_MaxPath];
    char FullPath[CimIO_MaxPath];
} watched_file;

typedef struct file_watcher_context
{
    char          Directory[CimIO_MaxPath];
    watched_file *Files;
    cim_u32       FileCount;
} file_watcher_context;

typedef struct cim_vtx 
{
    cim_vector2 Pos;
    cim_vector2 Tex;
    cim_vector4 Col;
} cim_vtx;

typedef struct cim_quad 
{
    cim_point_node *First;
    cim_vector4     Color;
} cim_quad;

typedef struct cim_batch 
{
    cim_u32       QuadsToRender;
    cim_bit_field Features;
    cim_rect      ClippingRect;
} cim_batch;

typedef struct cim_quad_stream 
{
    cim_quad *Source;
    cim_u32   Capacity;
    cim_u32   WriteOffset;
    cim_u32   ReadOffset;
} cim_quad_stream;

typedef struct cim_draw_command 
{
    size_t  VtxOffset;
    size_t  IdxOffset;
    cim_u32 IdxCount;
    cim_u32 VtxCount;

    cim_rect      ClippingRect;
    cim_bit_field Features;
} cim_draw_command;

typedef struct cim_command_stream 
{
    cim_draw_command *Source;
    cim_u32           Capacity;
    cim_u32           WriteOffset;
    cim_u32           ReadOffset;
} cim_command_stream;

typedef struct cim_command_buffer 
{
    cim_quad_stream    Quads;
    cim_command_stream Commands;
    cim_arena          FrameVtx;
    cim_arena          FrameIdx;
    cim_arena          Batches;
    bool ClippingRectChanged;
    bool FeatureStateChanged;
} cim_command_buffer;

typedef enum Component_Flag
{
    Component_Invalid = 0,
    Component_Window  = 1 << 0,
    Component_Button  = 1 << 1,
} Component_Flag;

typedef enum StyleUpdate_Flag
{
    StyleUpdate_Unknown        = 0,
    StyleUpdate_BorderGeometry = 1 << 0,
} StyleUpdate_Flag;

typedef struct window 
{
    struct
    {
        cim_vector4 HeadColor; // Think colors can be compressed. (4bits/Col?)
        cim_vector4 BodyColor; // Think colors can be compressed. (4bits/Col?)

        bool        HasBorders;
        cim_u32     BorderWidth;
        cim_vector4 BorderColor;
    } Style;

    bool IsClosed;

    cim_point_node *Head;
    cim_point_node *Body;
    cim_point_node *Border;
} window;

typedef struct button
{
    struct
    {
        cim_vector4 BodyColor;

        bool        HasBorders;
        cim_u32     BorderWidth;
        cim_vector4 BorderColor;

        cim_vector2 Position;
        cim_vector2 Dimension;
    } Style;

    // TODO: Change these to quads to see if the data flow is easier.
    cim_point_node *Body;
    cim_point_node *Border;
} button;

typedef struct component
{
    bool IsInitialized;

    Component_Flag Type;
    union
    {
        window Window;
        button Button;
    } For;

    cim_bit_field StyleUpdateFlags;
} component;

typedef enum QueryComponent_Flag
{
    QueryComponent_NoFlags        = 0,
    QueryComponent_IsTreeRoot     = 1 << 0,
    QueryComponent_AvoidHierarchy = 1 << 1,
} QueryComponent_Flag;

typedef struct cim_draggable 
{
    struct cim_point_node *PointRings[4];
    cim_u32 Count;
} cim_draggable;

extern cim_draggable Drag[4];
extern cim_u32       DragCount;

typedef enum CimLog_Severity
{
    CimLog_Info,
    CimLog_Warning,
    CimLog_Error,
    CimLog_Fatal,
} CimLog_Severity;

typedef void CimLogFn(CimLog_Severity Severity, const char *File, cim_i32 Line, const char *Format, va_list Args);
extern CimLogFn *CimLogger;

void
Cim_Log(CimLog_Severity Level, const char *File, cim_i32 Line, const char *Format, ...);

#define CimLog_Info(...)  Cim_Assert(CimLogger); Cim_Log(CimLog_Info   , __FILE__, __LINE__, __VA_ARGS__);
#define CimLog_Warn(...)  Cim_Assert(CimLogger); Cim_Log(CimLog_Warning, __FILE__, __LINE__, __VA_ARGS__)
#define CimLog_Error(...) Cim_Assert(CimLogger); Cim_Log(CimLog_Error  , __FILE__, __LINE__, __VA_ARGS__)
#define CimLog_Fatal(...) Cim_Assert(CimLogger); Cim_Log(CimLog_Fatal  , __FILE__, __LINE__, __VA_ARGS__)

typedef struct cim_context
{
    void *Backend;
    cim_command_buffer  CmdBuffer;
    cim_inputs          Inputs;
    cim_primitive_rings PrimitiveRings;

    // NOTE: Opaque pointers instead?
    struct ctree *CTree;
} cim_context;

extern cim_context *CimContext;

// [FUNCTIONS]

// Primitive operations

cim_point_node  *AllocateQuad  (cim_point At, cim_f32 Width, cim_f32 Height);
void             ReplaceQuad   (cim_point_node *ToReplace, cim_point TopLeft, cim_f32 Width, cim_f32 Height);

// Memory arenas
void    *CimArena_Push      (size_t Size, cim_arena *Arena);
void    *CimArena_GetLast   (size_t TypeSize, cim_arena *Arena);
cim_u32  CimArena_GetCount  (size_t TypeSize, cim_arena *Arena);
void     CimArena_Reset     (cim_arena *Arena);
void     CimArena_End       (cim_arena *Arena);

// Input queries
bool         IsMouseDown               (CimMouse_Button MouseButton);
bool         IsMouseReleased           (CimMouse_Button MouseButton);
bool         IsMouseClicked            (CimMouse_Button MouseButton);
cim_i32      CimInput_GetMouseDeltaX   (cim_inputs *Inputs);
cim_i32      CimInput_GetMouseDeltaY   (cim_inputs *Inputs);
cim_vector2  GetMousePosition          ();

// Command streams
cim_quad         *CimQuadStream_Read        (cim_u32 ReadCount, cim_quad_stream *Stream);
void              CimQuadStream_Write       (cim_u32 WriteCount, cim_quad *Quads, cim_quad_stream *Stream);
void              CimQuadStream_Reset       (cim_quad_stream *Stream);
void              DrawQuad                  (cim_point_node *Point, cim_vector4 Color);
void              CimCommandStream_Write    (cim_u32 WriteCount, cim_draw_command *Commands, cim_command_stream *Stream);
cim_draw_command *CimCommandStream_Read     (cim_u32 ReadCount, cim_command_stream *Stream);
void              CimCommandStream_Reset    (cim_command_stream *Stream);
void              CimCommand_BuildDrawData  (cim_command_buffer *CmdBuffer);

// Components
component  *QueryComponent  (const char *Key, cim_bit_field Flags);

// Constraints
void           CimConstraint_Solve  (cim_inputs *Inputs);

// Geometry
bool  IsInsideRect  (cim_rect Rect);

// Hashing
cim_u32        CimHash_FindFirstBit32  (cim_u32 Mask);
cim_u32        CimHash_String32        (const char *String);
cim_u32        CimHash_Block32         (void *Input, cim_u32 ByteLength);

// Style 
void CimStyle_Initialize(const char *File);

#ifdef __cplusplus
}
#endif
