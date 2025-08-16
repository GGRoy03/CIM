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
#define Cim_ArrayCount(Arr) (sizeof(Arr) / sizeof((Arr)[0]))
#define Cim_IsPowerOfTwo(Value) (((Value) & ((Value) - 1)) == 0)
#define Cim_SafeRatio(A, B) ((B) ? ((A)/(B)) : (A))

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

// Header files
#include "interface/cim_platform.h"
#include "interface/cim_helpers.h"
#include "interface/cim_layout.h"
#include "interface/cim_renderer.h"
#include "interface/cim_style.h"

typedef enum CimContext_State
{
    CimContext_Invalid     = 0,
    CimContext_Layout      = 1,
    CimContext_Interaction = 2,
} CimContext_State;

typedef struct cim_context
{
    CimContext_State State;

    cim_layout     Layout;
    cim_renderer   Renderer;
    cim_inputs     Inputs;
    cim_cmd_buffer Commands;

    void *Backend;
} cim_context;

static cim_context *CimCurrent;

#define UI_STATE      (CimCurrent->State)
#define UI_LAYOUT     (CimCurrent->Layout)
#define UIP_LAYOUT   &(CimCurrent->Layout)
#define UI_INPUT      (CimCurrent->Inputs)
#define UIP_INPUT    &(CimCurrent->Inputs)
#define UI_COMMANDS   (CimCurrent->Commands)
#define UIP_COMMANDS &(CimCurrent->Commands)
#define UI_RENDERER   (CimCurrent->Renderer)
#define UIP_RENDERER &(CimCurrent->Renderer)

// Implementation files
#include "implementation/cim_platform.cpp"
#include "implementation/cim_helpers.cpp"
#include "implementation/cim_layout.cpp"
#include "implementation/cim_renderer.cpp"
#include "implementation/cim_component.cpp"
#include "implementation/cim_styles.cpp"

// WARN: Not sure where to put these / if we need them.

static void
BeginUIFrame()
{
    // Obviously wouldn't really do this but prototyping.
    cim_layout *Layout = UIP_LAYOUT;
    Layout->Tree.NodeCount = 0;
    Layout->Tree.AtParent = 0;
    Layout->Tree.ParentStack[0] = CimLayout_InvalidNode; // Temp.

    UI_STATE = CimContext_Layout;
}

static void
EndUIFrame()
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

static bool
InitUIContext(cim_context *UserContext)
{
    if (!UserContext)
    {
        return false;
    }

    memset(UserContext, 0, sizeof(cim_context));
    CimCurrent = UserContext;

    return true;
}