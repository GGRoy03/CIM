#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <immintrin.h>

#define Cim_Assert(cond) do { if (!(cond)) __debugbreak(); } while (0)

typedef uint8_t  cim_u8;
typedef uint32_t cim_u32;
typedef int      cim_i32;

typedef cim_u32 cim_bit_field;


#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ============================================================
// PUBLIC API TYPE DEFINITIONS FOR CIM. BY SECTION.
// -[SECTION]: Material
// ============================================================
// ============================================================

// -[SECTION:Material] {

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

cim_texture Cim_LoadTextureFromDisk(const char *FileName);

void Cim_CreateMaterial(const char *ID, cim_bit_field Features);
void Cim_DestroyMaterial(const char *ID);

// } -[SECTION:Materia]

bool Window (const char *Id);
bool Button (const char *Id);
void Text   (const char *Id);


#ifdef __cplusplus
}
#endif