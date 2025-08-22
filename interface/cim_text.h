#pragma once

#define DEBUG_VALIDATE_LRU 0

typedef struct glyph_size
{
    cim_u16 Width;
    cim_u16 Height;
}  glyph_size;

typedef struct glyph_info
{
    bool       IsInAtlas;
    cim_f32    U0, V0, U1, V1;
    glyph_size Size;
} glyph_info;

typedef struct glyph_layout_info
{
    cim_f32 OffsetX;
    cim_f32 OffsetY;
    cim_f32 AdvanceX;
} glyph_layout_info;

// General layout for a glyph run, maybe rename?
typedef struct text_layout_info
{
    // Global
    void   *BackendLayout;
    cim_u16 Width;
    cim_u16 Height;
    cim_u32 LineHeight; // WARN: This is font-related not actually layout related.

    // Per glyph
    glyph_layout_info *GlyphLayoutInfo;
    cim_u32            GlyphCount;
} text_layout_info;

static void         InitGlyphCache  (void);
static glyph_info * GetGlyphInfo    (char Character);