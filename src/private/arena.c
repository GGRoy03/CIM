#include "cim_private.h"

#include <string.h> // For memcpy

#ifdef __cplusplus
extern "C" {
#endif

void *
CimArena_Push(size_t     Size,
              cim_arena *Arena)
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
            memset((char*)New + Arena->At, 0, Arena->Capacity - Arena->At);
        }

        if (Arena->Memory)
        {
            memcpy(New, Arena->Memory, Arena->At);
            free(Arena->Memory);
        }

        Arena->Memory = New;
    }

    void *Ptr  = (char*)Arena->Memory + Arena->At;
    Arena->At += Size;

    return Ptr;
}

void *
CimArena_GetLast(size_t     TypeSize,
                 cim_arena *Arena)
{
    Cim_Assert(Arena->Memory);
    return (char*)Arena->Memory + (Arena->At - TypeSize);
}

cim_u32
CimArena_GetCount(size_t     TypeSize,
                  cim_arena *Arena)
{
    Cim_Assert(Arena->Memory);
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

        Arena->Memory   = NULL;
        Arena->At       = 0;
        Arena->Capacity = 0;
    }
}

#ifdef __cplusplus
}
#endif
