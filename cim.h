#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <immintrin.h>

#define Cim_Assert(cond) do { if (!(cond)) __debugbreak(); } while (0)

typedef uint8_t  cim_u8;
typedef uint32_t cim_u32;
typedef int      cim_i32;


#ifdef __cplusplus
extern "C" {
#endif

bool Window (const char *Id);
bool Button (const char *Id);
void Text   (const char *Id);


#ifdef __cplusplus
}
#endif