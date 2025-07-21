#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <immintrin.h>

#define Cim_Assert(cond) do { if (!(cond)) __debugbreak(); } while (0)

typedef uint8_t  cim_u8;
typedef uint32_t cim_u32;
typedef int      cim_i32;
typedef float    cim_f32;
typedef double   cim_f64;

typedef cim_u32 cim_bit_field;


#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ============================================================
// PUBLIC API TYPE DEFINITIONS FOR CIM. BY SECTION.
// -[SECTION]: Material
// -[SECTION]: IO
// -[SECTION]: Widgets
// ============================================================
// ============================================================

// [SECTION:Material] {

typedef enum CimMaterialFeature_Type
{
	CimMaterialFeature_Color = 1 << 0,
} CimMaterialFeature_Type;

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

typedef enum CimMouse_Button 
{
    CimMouse_Left,
    CimMouse_Right,
    CimMouse_Middle,

    CimMouse_ButtonCount,
} CimMouse_Button;

typedef struct cim_io_inputs
{
    cim_io_button_state Buttons[CIM_KEYBOARD_KEY_COUNT];

    cim_f32 ScrollDelta;
    cim_i32 MouseX, MouseY;
    cim_f32 MouseDeltaX, MouseDeltaY;
    cim_io_button_state MouseButtons[5];
} cim_io_inputs;

bool        Cim_IsMouseDown(CimMouse_Button MouseButton);
bool        Cim_IsMouseReleased(CimMouse_Button MouseButton);
cim_vector2 Cim_GetMousePosition();
cim_f32     Cim_GetMouseDeltaX();
cim_f32     Cim_GetMouseDeltaY();

// } [SECTION:IO]

// [SECTION:Widgets] {

typedef enum CimWindow_Flags
{
    CimWindow_Draggable,
} CimWindow_Flags;

bool Window (const char *Id, cim_bit_field Flags);

// } [SECTION:Widgets]


#ifdef __cplusplus
}
#endif
