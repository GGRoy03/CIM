#include <stdint.h>
#include <stdbool.h>
#include <immintrin.h>

#include <string.h> // For memcpy
#include <float.h>  // For numeric limits

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


// TODO: Rethink the structure a bit. The platform needs some more information
// Such as inputs structure to properly work. So we need a further separation. Base?  
// Some sort of platform interface. C File?

// Internal (A lot of files?)
#include "private/hashing.c"
#include "private/arena.c"    // NOTE: Probably do not really need those.
#include "private/geometry.c"
#include "private/io.c"
#include "private/primitives.c"
#include "private/components.c"
#include "private/commands.c"
#include "private/style.c"
#include "private/constraints.c"

// App context which acts as a bridge between the private code and the public code.

typedef struct cim_context
{
    void                 *Backend;
    cim_command_buffer    CmdBuffer;
    cim_inputs            Inputs;
    cim_component_hashmap ComponentStore;
    cim_primitive_rings   PrimitiveRings;
} cim_context;

static cim_context *CimContext;

// Public
#include "public/components.c"
#include "public/features.c"
