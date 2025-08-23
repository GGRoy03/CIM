#pragma once

typedef enum CimRenderer_Backend
{
	CimRenderer_D3D = 0,
} CimRenderer_Backend;

typedef enum UIShader_Type
{
    UIShader_Vertex = 0,
    UIShader_Pixel  = 1,
} UIShader_Type;

typedef enum UIPipeline_Type
{
    UIPipeline_None    = 0,
    UIPipeline_Default = 1,
    UIPipeline_Text    = 2,
    UIPipeline_Count   = 3,
} UIPipeline_Type;

typedef struct ui_shader_desc
{
    char         *SourceCode;
    UIShader_Type Type;
} ui_shader_desc;

// NOTE: I don't really know if we want that.
typedef enum CimFeature_Type
{
    CimFeature_None        = 0,
	CimFeature_AlbedoMap   = 1 << 0,
	CimFeature_MetallicMap = 1 << 1,
    CimFeature_Text        = 1 << 2,
	CimFeature_Count       = 3,
} CimFeature_Type;

typedef struct cim_draw_command
{
    size_t VertexByteOffset;
    size_t BaseVertexOffset;
    size_t StartIndexRead;

    cim_u32 IdxCount;
    cim_u32 VtxCount;

    cim_rect        ClippingRect;
    cim_bit_field   Features;
    UIPipeline_Type PipelineType;
} cim_draw_command;

typedef struct cim_cmd_buffer
{
    cim_u32   GlobalVtxOffset;
    cim_u32   GlobalIdxOffset;

    cim_arena FrameVtx;
    cim_arena FrameIdx;

    cim_draw_command *Commands;
    cim_u32           CommandCount;
    cim_u32           CommandSize;

    cim_rect        CurrentClipRect;
    UIPipeline_Type CurrentPipelineType;
} cim_command_buffer;

typedef void DrawUI      (cim_i32 Width, cim_i32 Height);
typedef void draw_glyph  (stbrp_rect Rect);
typedef struct cim_renderer 
{
	DrawUI     *Draw;
    draw_glyph *TransferGlyph;
} cim_renderer;