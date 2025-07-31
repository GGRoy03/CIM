#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <immintrin.h>

#define Cim_Assert(Cond) do { if (!(Cond)) __debugbreak(); } while (0)

typedef uint8_t  cim_u8;
typedef uint32_t cim_u32;
typedef int      cim_i32;
typedef float    cim_f32;
typedef double   cim_f64;

typedef cim_u32 cim_bit_field;

#ifdef __cplusplus
extern "C" {
#endif

// TODO: Rework formatting of this file.

// ============================================================
// ============================================================
// PUBLIC API TYPE DEFINITIONS FOR CIM. BY SECTION.
// -[SECTION]: Material
// -[SECTION]: IO
// -[SECTION]: Components
// ============================================================
// ============================================================

// [SECTION:Material] {

typedef enum CimFeature_Type
{
    CimFeature_AlbedoMap   = 1 << 0,
    CimFeature_MetallicMap = 1 << 1,
} CimFeature_Type;

#define CimFeature_Count 2

typedef struct cim_texture
{
    cim_u8 *Pixels;
    size_t  DataSize;
    cim_i32 Width;
    cim_i32 Height;
    cim_i32 Pitch;
    cim_i32 Channels;
} cim_texture;

// } [SECTION:Material]

// [SECTION:IO] {

#define CIM_KEYBOARD_KEY_COUNT 256
#define CIM_KEYBOARD_EVENT_BUFFER_COUNT 128

typedef struct cim_io_button_state
{
    bool    EndedDown;
    cim_u32 HalfTransitionCount;
} cim_io_button_state;

typedef struct cim_keyboard_event
{
    cim_u8 VKCode;
    bool   IsDown;
} cim_keyboard_event;

typedef struct cim_vector2
{
    cim_f32 x, y;
} cim_vector2;

typedef struct cim_vector4
{
    cim_f32 x, y, w, z;
} cim_vector4;

typedef enum CimMouse_Button 
{
    CimMouse_Left,
    CimMouse_Right,
    CimMouse_Middle,

    CimMouse_ButtonCount,
} CimMouse_Button;

// NOTE: This name is stupid.
typedef struct cim_io_inputs
{
    cim_io_button_state Buttons[CIM_KEYBOARD_KEY_COUNT];

    cim_f32 ScrollDelta;
    cim_i32 MouseX, MouseY;
    cim_i32 MouseDeltaX, MouseDeltaY;
    cim_io_button_state MouseButtons[5];
} cim_io_inputs;

bool        CimInput_IsMouseDown(CimMouse_Button MouseButton);
bool        CimInput_IsMouseReleased(CimMouse_Button MouseButton);
bool        CimInput_IsMouseClicked(CimMouse_Button MouseButton);
cim_vector2 CimInput_GetMousePosition(void);
cim_i32     CimInput_GetMouseDeltaX(void);
cim_i32     CimInput_GetMouseDeltaY(void);

// } [SECTION:IO]

// [SECTION:Components] {

void
Cim_EndFrame();

typedef enum CimWindow_Flags
{
    CimWindow_Draggable = 1 << 0,
} CimWindow_Flags;

#define Window(Id, Color, Flags) Cim_Window(Id, Color, Flags)

bool Cim_Window(const char *Id, cim_vector4 Color, cim_bit_field Flags);

// } [SECTION:Components]

#ifdef __cplusplus
}
#endif
