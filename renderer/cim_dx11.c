#include "cim_dx11.h"

// ============================================================
// ============================================================
// DX11 IMPLEMENTATION FOR CIM. BY SECTION.
// -[SECTION] Hashing
// ============================================================
// ============================================================

// -[SECTION] Hashing {
// 1) Contains a lot of duplicate code for now. That is because the full design is not yet set in stone.

static void CimDx11_InitializeGroupMap(cim_dx11_group_map *Map)
{
    Map->GroupCount = 8;

    cim_u32 BucketCount = Map->GroupCount * Dx11MapBucketGroupSize;
    size_t  BucketAllocationSize = BucketCount * sizeof(cim_dx11_group_entry);

    Map->Buckets = malloc(BucketAllocationSize);
    Map->MetaData = malloc(BucketCount * sizeof(cim_u8));

    if (!Map->Buckets || !Map->MetaData)
    {
        return;
    }

    memset(Map->Buckets, 0, BucketAllocationSize);
    memset(Map->MetaData, Dx11MapEmptyBucketTag, BucketCount * sizeof(cim_u8));
}

static void CimDx11_InsertGroup(cim_dx11_group_map *Map, const char *Key, cim_dx11_group Value)
{
    cim_u32 ProbeCount  = 0;
    cim_u32 HashedValue = Cim_HashString(Key);
    cim_u32 GroupIndex  = HashedValue & (Map->GroupCount - 1);

    while (true)
    {
        cim_u8 *Meta = Map->MetaData + (GroupIndex * Dx11MapBucketGroupSize);
        cim_u8  Tag = (HashedValue & 0x7F);

        Cim_Assert(Tag < Dx11MapEmptyBucketTag);

        __m128i MetaVector = _mm_loadu_si128((__m128i *)Meta);
        __m128i TagVector = _mm_set1_epi8(Tag);

        cim_i32 Mask = _mm_movemask_epi8(_mm_cmpeq_epi8(MetaVector, TagVector));

        while (Mask)
        {
            cim_u32 Lane  = Cim_FindFirstBit32(Mask);
            cim_u32 Index = (GroupIndex * Dx11MapBucketGroupSize) + Lane;

            cim_dx11_group_entry *Entry = Map->Buckets + Index;
            if (Entry->Key == Key)
            {
                return;
            }

            Mask &= Mask - 1;
        }

        __m128i EmptyVector = _mm_set1_epi8(Dx11MapEmptyBucketTag);
        cim_i32 MaskEmpty = _mm_movemask_epi8(_mm_cmpeq_epi8(MetaVector,
            EmptyVector));

        if (MaskEmpty)
        {
            cim_i32 Lane  = Cim_FindFirstBit32(MaskEmpty);
            cim_u32 Index = (GroupIndex * Dx11MapBucketGroupSize) + Lane;

            cim_dx11_group_entry *Entry = Map->Buckets + Index;
            strcpy_s(Entry->Key, sizeof(Entry->Key), Key);
            Entry->Value = Value;

            Meta[Lane] = Tag;

            return;
        }

        ProbeCount++;
        GroupIndex = (GroupIndex + (ProbeCount * ProbeCount)) & (Map->GroupCount - 1);
    }
}

static cim_dx11_group CimDx11_GetGroup(cim_dx11_group_map *Map, const char *Key)
{
	cim_u32 ProbeCount = 0;
	cim_u32 Hashed     = Cim_HashString(Key);
	cim_u32 GroupIndex = Hashed & (Map->GroupCount - 1);

	while (true)
	{
		cim_u8 *Meta = Map->MetaData + (GroupIndex * Dx11MapBucketGroupSize);
		cim_u8  Tag  = (Hashed & 0x7F);

		__m128i MetaVector = _mm_loadu_si128((__m128i *)Meta);
        __m128i TagVector  = _mm_set1_epi8(Tag);

        cim_i32 Mask = _mm_movemask_epi8(_mm_cmpeq_epi8(MetaVector, TagVector));

        while (Mask)
        {
            cim_u32 Lane  = Cim_FindFirstBit32(Mask);
            cim_u32 Index = (GroupIndex * Dx11MapBucketGroupSize) + Lane;

            cim_dx11_group_entry *Entry = Map->Buckets + Index;
            if (Entry->Key == Key)
            {
                return Entry->Value;
            }

            Mask &= Mask - 1;
        }

        __m128i EmptyVector = _mm_set1_epi8(Dx11MapEmptyBucketTag);
        cim_i32 MaskEmpty   = _mm_movemask_epi8(_mm_cmpeq_epi8(MetaVector, EmptyVector));

        if (MaskEmpty)
        {
            abort();
            return (cim_dx11_group){ 0 };
        }

        ProbeCount++;
        GroupIndex = (GroupIndex + (ProbeCount * ProbeCount)) & (Map->GroupCount - 1);
	}
}

// } -[SECTION:Hashing]

void CimDx11_Initialize(cim_context *Ctx, ID3D11Device *UserDevice, ID3D11DeviceContext *UserContext)
{
    cim_dx11_backend *Backend = malloc(sizeof(cim_dx11_backend));

    if (!Backend)
    {
        return;
    }

    Backend->Device        = UserDevice;
    Backend->DeviceContext = UserContext;
    CimDx11_InitializeGroupMap(&Backend->GroupMap);
    Ctx->Backend = Backend;

    // NOTE: Here's the tricky part, we have to create the material.
    // I think we just use the same or almost the same scheme we used
    // for the engine.

}