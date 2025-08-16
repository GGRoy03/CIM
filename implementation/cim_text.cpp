namespace CimText
{

#define CimText_ValidateGlyphLRU 0

typedef struct gpu_glyph_index
{
    cim_u32 Value;
} cim_gpy_glyph_index;

typedef struct glyph_cache_pointt
{
    cim_u32 X;
    cim_u32 Y;
} glyph_cache_pointt;

typedef struct glyph_table_params
{
    cim_u32 HashCount;
    cim_u32 EntryCount;
    cim_u32 ReservedTileCount;
    cim_u32 CacheTileCountInX;
} glyph_table_params;

typedef struct glyph_hash
{
    __m128i Value;
} glyph_hash;

typedef struct glyph_state
{
    cim_u32             Id;
    gpu_glyph_index GPUIdx;

    cim_u32 FilledState;
    cim_u16 DimX;
    cim_u16 DimY;
} glyph_state;

typedef struct glyph_table_stats
{
    size_t HitCount;
    size_t MissCount;
    size_t RecycleCount;
} glyph_table_stats;

typedef struct glyph_entry
{
    glyph_hash HashValue;

    cim_u32         NextWithSameHash;
    cim_u32         NextLRU;
    cim_u32         PrevLRU;
    gpu_glyph_index GPUIdx;

    cim_u32 FilledState;
    cim_u16 DimX;
    cim_u16 DimY;

#ifdef CimText_ValidateGlyphLRU
    size_t Ordering;
#endif
} glyph_entry;

typedef struct glyph_table
{
    glyph_table_stats Stats;

    cim_u32 HashMask;
    cim_u32 HashCount;
    cim_u32 EntryCount;

    cim_u32     *HashTable;
    glyph_entry *Entries;

#ifdef CimText_ValidateGlyphLRU
    cim_u32 LastLRUCount;
#endif
} glyph_table;

static gpu_glyph_index
PackGlyphCachePoint(cim_u32 X, cim_u32 Y)
{
    gpu_glyph_index Result = { (Y << 16) | X };
    return Result;
}

static glyph_cache_pointt
UnpackGlyphCachePoint(gpu_glyph_index P)
{
    glyph_cache_pointt Result;

    Result.X = (P.Value & 0xffff);
    Result.Y = (P.Value >> 16);

    return Result;
}

static cim_i32
GlyphHashesAreEqual(glyph_hash A, glyph_hash B)
{
    __m128i Compare = _mm_cmpeq_epi32(A.Value, B.Value);
    cim_i32 Result  = (_mm_movemask_epi8(Compare) == 0xffff);
    return Result;
}

static cim_u32 *
GetSlotPointer(glyph_table *Table, glyph_hash RunHash)
{
    // NOTE: Index is stored in lower 32-bits of the hash.
    cim_u32 HashIndex = _mm_cvtsi128_si32(RunHash.Value);
    cim_u32 HashSlot  = (HashIndex & Table->HashMask);

    Cim_Assert(HashSlot < Table->HashCount);
    cim_u32 *Result = &Table->HashTable[HashSlot];
    return Result;
}

static glyph_entry *
GetEntry(glyph_table *Table, cim_u32 Idx)
{
    Cim_Assert(Idx < Table->EntryCount);
    glyph_entry *Result = Table->Entries + Idx;
    return Result;
}

static glyph_entry *
GetSentinel(glyph_table *Table)
{
    glyph_entry *Result = Table->Entries;
    return Result;
}

static glyph_table_stats
GetAndClearStats(glyph_table *Table)
{
    glyph_table_stats Result    = Table->Stats;
    glyph_table_stats ZeroStats = {};

    Table->Stats = ZeroStats;

    return Result;
}

static void
UpdateGlyphCacheEntry(glyph_table *Table, cim_u32 Id, cim_u32 NewState, cim_u16 NewDimX, cim_u16 NewDimY)
{
    glyph_entry *Entry = GetEntry(Table, Id);

    Entry->FilledState = NewState;
    Entry->DimX        = NewDimX;
    Entry->DimY        = NewDimY;
}

#if CimText_ValidateGlyphLRU 
static void
    ValidateLRU(glyph_table *Table, cim_i32 ExpectedCountChange)
{
    cim_u32 EntryCount = 0;

    glyph_entry *Sentinel = GetSentinel(Table);
    size_t           LastOrdering = Sentinel->Ordering;

    for (uint32_t EntryIndex = Sentinel->NextLRU; EntryIndex != 0)
    {
        glyph_entry *Entry = GetEntry(Table, EntryIndex);
        Cim_Assert(Entry->Ordering < LastOrdering);

        LastOrdering = Entry->Ordering;
        EntryIndex = Entry->NextLRU;

        ++EntryCount;
    }

    if ((Table->LastLRUCount + ExpectedCountChange) != EntryCount)
    {
        __debugbreak();
    }

    Table->LastLRUCount = EntryCount;
}
#else
#define ValidateLRU(...)
#endif

static cim_u32
PopFreeEntry(glyph_table *Table)
{
    glyph_entry *Sentinel = GetSentinel(Table);

    // NOTE: Means we are using every entry.
    if (!Sentinel->NextWithSameHash)
    {
        // RecycleLRU(Table);
        __debugbreak();
    }

    cim_u32 Result = Sentinel->NextWithSameHash; // NOTE: Linked list again.
    Cim_Assert(Result);

    glyph_entry *Entry = GetEntry(Table, Result);
    Sentinel->NextWithSameHash = Entry->NextWithSameHash;
    Entry->NextWithSameHash    = 0;

    Cim_Assert(Entry);
    Cim_Assert(Entry != Sentinel);
    Cim_Assert(Entry->DimX == 0);
    Cim_Assert(Entry->DimY == 0);
    Cim_Assert(Entry->FilledState == 0);
    Cim_Assert(Entry->NextWithSameHash == 0);
    Cim_Assert(Entry == GetEntry(Table, Result));

    return Result;
}

static glyph_state
FindGlyphEntryByHash(glyph_table *Table, glyph_hash RunHash)
{
    glyph_entry *Result = 0;

    cim_u32 *Slot = GetSlotPointer(Table, RunHash);
    cim_u32  EntryIndex = *Slot;

    while (EntryIndex)
    {
        glyph_entry *Entry = GetEntry(Table, EntryIndex);
        if (GlyphHashesAreEqual(Entry->HashValue, RunHash))
        {
            Result = Entry;
            break;
        }

        EntryIndex = Entry->NextWithSameHash;
    }

    if (Result)
    {
        Cim_Assert(EntryIndex);

        glyph_entry *Prev = GetEntry(Table, Result->PrevLRU);
        glyph_entry *Next = GetEntry(Table, Result->NextLRU);

        Prev->NextLRU = Result->NextLRU;
        Next->PrevLRU = Result->PrevLRU;

        ValidateLRU(Table, -1);

        ++Table->Stats.HitCount;
    }
    else
    {
        EntryIndex = PopFreeEntry(Table);
        Cim_Assert(EntryIndex);

        Result = GetEntry(Table, EntryIndex);
        Cim_Assert(Result->FilledState == 0);
        Cim_Assert(Result->NextWithSameHash == 0);
        Cim_Assert(Result->DimX == 0);
        Cim_Assert(Result->DimY == 0);

        Result->NextWithSameHash = *Slot;
        Result->HashValue = RunHash;

        *Slot = EntryIndex;

        ++Table->Stats.MissCount;
    }

    glyph_entry *Sentinel = GetSentinel(Table);
    Cim_Assert(Result != Sentinel);
    Result->NextLRU = Sentinel->NextLRU;
    Result->PrevLRU = 0;

    glyph_entry *NextLRU = GetEntry(Table, Sentinel->NextLRU);
    NextLRU->PrevLRU  = EntryIndex;
    Sentinel->NextLRU = EntryIndex;

#if CimText_ValidateGlyphLRU
    Result->Ordering = Sentinel->Ordering++;
#endif
    ValidateLRU(Table, 1);

    glyph_state State;
    State.Id          = EntryIndex;
    State.DimX        = Result->DimX;
    State.DimY        = Result->DimY;
    State.GPUIdx      = Result->GPUIdx;
    State.FilledState = Result->FilledState;

    return State;
}

static glyph_table *
PlaceGlyphTableInMemory(glyph_table_params Params, void *Memory)
{
    Cim_Assert(Params.HashCount  >= 1);
    Cim_Assert(Params.EntryCount >= 2);
    Cim_Assert(Cim_IsPowerOfTwo(Params.HashCount));
    Cim_Assert(Params.CacheTileCountInX >= 1);

    glyph_table *Result = 0;

    if (Memory)
    {
        glyph_entry *Entries = (glyph_entry *)Memory;

        Result = (glyph_table *)(Entries + Params.EntryCount);
        Result->HashTable = (uint32_t *)(Result + 1);
        Result->Entries   = Entries;

        Result->HashMask   = Params.HashCount - 1;
        Result->HashCount  = Params.HashCount;
        Result->EntryCount = Params.EntryCount;

        memset(Result->HashTable, 0, Result->HashCount * sizeof(Result->HashTable[0]));

        cim_u32 StartingTile = Params.ReservedTileCount;
        cim_u32 X            = StartingTile % Params.CacheTileCountInX;
        cim_u32 Y            = StartingTile / Params.CacheTileCountInX;

        for (cim_u32 EntryIndex = 0; EntryIndex < Params.EntryCount; ++EntryIndex)
        {
            if (X >= Params.CacheTileCountInX)
            {
                X = 0;
                ++Y;
            }

            glyph_entry *Entry = GetEntry(Result, EntryIndex);
            if ((EntryIndex + 1) < Params.EntryCount)
            {
                Entry->NextWithSameHash = EntryIndex + 1;
            }
            else
            {
                Entry->NextWithSameHash = 0;
            }

            Entry->GPUIdx      = PackGlyphCachePoint(X, Y);
            Entry->FilledState = 0;
            Entry->DimX        = 0;
            Entry->DimY        = 0;

            ++X;
        }

        GetAndClearStats(Result);
    }

    return Result;
} 

}
