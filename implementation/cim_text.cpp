#define CimText_AtlasWidth 1024
#define CimText_AtlasHeight 1024

static glyph_info    GlyphCache[256];
static stbrp_context AtlasContext;
static stbrp_node    AtlasNodes[CimText_AtlasWidth];

static void
InitGlyphCache(void)
{
	stbrp_init_target(&AtlasContext, CimText_AtlasWidth, CimText_AtlasHeight, AtlasNodes, CimText_AtlasWidth);
}

static glyph_info *
GetGlyphInfo(char Character)
{
	glyph_info *Info = &GlyphCache[(cim_u32)Character];

	if (!Info->IsInAtlas)
	{
		glyph_size GlyphSize = OSGetTextExtent(&Character, 1);
		if (GlyphSize.Width == 0 || GlyphSize.Height == 0)
		{
			return NULL;
		}
		Info->Size = GlyphSize;

		stbrp_rect Rect;
		Rect.id = 0;
		Rect.w = GlyphSize.Width;
		Rect.h = GlyphSize.Height;

		stbrp_pack_rects(&AtlasContext, &Rect, 1);
		if (!Rect.was_packed)
		{
			return NULL;
		}

		Info->U0 = (cim_f32) Rect.x / CimText_AtlasWidth;
		Info->V0 = (cim_f32) Rect.y / CimText_AtlasHeight;
		Info->U1 = (cim_f32)(Rect.x + Rect.w) / CimText_AtlasWidth;
		Info->V1 = (cim_f32)(Rect.y + Rect.h) / CimText_AtlasHeight;

		OSRasterizeGlyph(Character, Rect);

		Info->IsInAtlas = true;
	}

	return Info;
}