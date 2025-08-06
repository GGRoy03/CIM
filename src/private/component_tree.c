#include "cim_private.h"

#include <string.h> // memset

#ifdef __cplusplus
extern "C" {
#endif

cim_component *
CimComponent_GetOrInsert(const char            *Key,
                         cim_component_hashmap *Hashmap)
{
    if (!Hashmap->IsInitialized)
    {
        Hashmap->GroupCount = 32;

        cim_u32 BucketCount  = Hashmap->GroupCount * CimBucketGroupSize;
        size_t  BucketSize   = BucketCount * sizeof(cim_component_entry);
        size_t  MetadataSize = BucketCount * sizeof(cim_u8);

        Hashmap->Buckets  = malloc(BucketSize);
        Hashmap->Metadata = malloc(MetadataSize);

        if (!Hashmap->Buckets || !Hashmap->Metadata)
        {
            return NULL;
        }

        memset(Hashmap->Buckets ,  0               , BucketSize);
        memset(Hashmap->Metadata, CimEmptyBucketTag, MetadataSize);

        Hashmap->IsInitialized = true;
    }

    cim_u32 ProbeCount = 0;
    cim_u32 Hashed     = CimHash_String32(Key);
    cim_u32 GroupIdx   = Hashed & (Hashmap->GroupCount - 1);

    while (true)
    {
        cim_u8 *Meta = Hashmap->Metadata + GroupIdx*CimBucketGroupSize;
        cim_u8  Tag  = (Hashed & 0x7F);

        __m128i Mv = _mm_loadu_si128((__m128i*)Meta);
        __m128i Tv = _mm_set1_epi8(Tag);

        cim_i32 TagMask = _mm_movemask_epi8(_mm_cmpeq_epi8(Mv, Tv));
        while (TagMask)
        {
            cim_u32 Lane = CimHash_FindFirstBit32(TagMask);
            cim_u32 Idx  = (GroupIdx * CimBucketGroupSize) + Lane;

            cim_component_entry *E = Hashmap->Buckets + Idx;
            if (strcmp(E->Key, Key) == 0)
            {
                return &E->Value;
            }

            TagMask &= TagMask - 1;
        }

        __m128i Ev        = _mm_set1_epi8(CimEmptyBucketTag);
        cim_i32 EmptyMask = _mm_movemask_epi8(_mm_cmpeq_epi8(Mv, Ev));
        if (EmptyMask)
        {
            cim_u32 Lane = CimHash_FindFirstBit32(EmptyMask);
            cim_u32 Idx  = (GroupIdx * CimBucketGroupSize) + Lane;

            cim_component_entry *E = Hashmap->Buckets + Idx;
            strcpy_s(E->Key, sizeof(E->Key), Key);
            E->Key[sizeof(E->Key)-1] = 0;

            Meta[Lane] = Tag;

            return &E->Value;
        }

        ProbeCount++;
        GroupIdx = (GroupIdx + ProbeCount * ProbeCount) & (Hashmap->GroupCount - 1);
    }
}

#ifdef __cplusplus
}
#endif
