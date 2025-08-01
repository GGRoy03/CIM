#pragma once

// [Types]

#include <stdint.h>
#include <stdbool.h>
#include <immintrin.h>

#define Cim_Assert(Cond) do { if (!(Cond)) __debugbreak(); } while (0)

typedef uint8_t  cim_u8;
typedef uint32_t cim_u32;

typedef int cim_i32;

typedef float  cim_f32;
typedef double cim_f64;

typedef cim_u32 cim_bit_field;

typedef struct cim_vector2
{
    cim_f32 x, y;
} cim_vector2;

typedef struct cim_vector4
{
    cim_f32 x, y, w, z;
} cim_vector4;

typedef struct cim_rect
{
    cim_vector2 Min;
    cim_vector2 Max;
} cim_rect;

// [Components]

typedef enum CimWindow_Flags
{
    CimWindow_Draggable = 1 << 0,
} CimWindow_Flags;

#ifdef CimDefault_UseWindowShortcut
#define Window(Id, Color, Flags) Cim_Window(Id, Color, Flags)
#endif

#ifdef __cplusplus
extern "C" {
#endif

bool
Cim_Window(const char *Id, cim_vector4 Color, cim_bit_field Flags);

#ifdef __cplusplus
}
#endif

// [Features]

#define CimFeature_Count 2

typedef enum CimFeature_Type
{
    CimFeature_AlbedoMap   = 1 << 0,
    CimFeature_MetallicMap = 1 << 1,
} CimFeature_Type;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif
