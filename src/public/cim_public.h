#pragma once

#include "private/cim_private.h"

// [Macros & Types]

#define CimFeature_Count 2

// [STRUCTS & Enums]

typedef enum CimWindow_Flags
{
    CimWindow_Draggable = 1 << 0,
} CimWindow_Flags;

typedef enum CimFeature_Type
{
    CimFeature_AlbedoMap = 1 << 0,
    CimFeature_MetallicMap = 1 << 1,
} CimFeature_Type;

// [FUNCTIONS]

#ifdef __cplusplus
extern "C" {
#endif

// Components

bool Cim_Window(const char *Id, cim_bit_field Flags);

// Remove this asap
void Cim_EndFrame();

#ifdef __cplusplus
}
#endif