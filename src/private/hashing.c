#include "private/cim_private.h"

#define FNV32Prime 0x01000193
#define FNV32Basis 0x811C9DC5

#ifdef __cplusplus
extern "C" {
#endif

cim_u32
CimHash_FindFirstBit32(cim_u32 Mask)
{
    return (cim_u32)__builtin_ctz(Mask);
}

cim_u32
CimHash_String32(const char* String)
{
    if (!String)
    {
        Cim_Assert(!"Cannot hash empty string.");
        return 0;
    }

    cim_u32 Result = FNV32Basis;
    cim_u8  Character;

    while ((Character = *String++))
    {
        Result = FNV32Prime * (Result ^ Character);
    }

    return Result;
}

cim_u32 
CimHash_Block32(void *ToHash, cim_u32 ToHashLength)
{
    cim_u8 *BytePointer = (cim_u8*)ToHash;
    cim_u32 Result      = FNV32Basis;

    if(!ToHash)
    {
        CimLog_Error("Called Hash32Bits with NULL memory adress as value.");
        return 0;
    }

    if(ToHashLength < 1)
    {
        CimLog_Error("Called Hash32Bits with length inferior to 1.");
        return 0;
    }

    while(ToHashLength--)
    {
        Result = FNV32Prime * (Result ^ *BytePointer++);
    }

    return Result;
}

#ifdef __cplusplus
}
#endif
