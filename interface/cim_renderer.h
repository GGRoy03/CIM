#pragma once

typedef enum CimRenderer_Backend
{
	CimRenderer_None = 0,
	CimRenderer_Dx11 = 1,
} CimRenderer_Backend;

typedef enum CimFeature_Type
{
	CimFeature_AlbedoMap   = 1 << 0,
	CimFeature_MetallicMap = 1 << 1,
    CimFeature_Text        = 1 << 2,
	CimFeature_Count       = 2,
} CimFeature_Type;

typedef struct cim_draw_command
{
    size_t  VtxOffset;
    size_t  IdxOffset;
    cim_u32 IdxCount;
    cim_u32 VtxCount;

    cim_rect      ClippingRect;
    cim_bit_field Features;
} cim_draw_command;

typedef struct cim_cmd_buffer
{
    cim_arena FrameVtx;
    cim_arena FrameIdx;

    cim_draw_command *Commands;
    cim_u32           CommandCount;
    cim_u32           CommandSize;

    bool ClippingRectChanged;
    bool FeatureStateChanged;
} cim_command_buffer;

typedef void DrawUI      (cim_i32 Width, cim_i32 Height);
typedef void draw_glyph  (stbrp_rect Rect);
typedef struct cim_renderer 
{
	DrawUI     *Draw;
    draw_glyph *TransferGlyph;
} cim_renderer;