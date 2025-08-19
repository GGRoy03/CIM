#pragma once

#define CimLayout_MaxNestDepth 32
#define CimLayout_InvalidNode 0xFFFFFFFF
#define CimLayout_MaxShapesForDrag 4
#define CimLayout_MaxDragPerBatch 16

typedef enum CimComponent_Flag
{
    CimComponent_Invalid = 0,
    CimComponent_Window = 1 << 0,
    CimComponent_Button = 1 << 1,
} CimComponent_Flag;

typedef enum Layout_Order
{
    Layout_Horizontal = 0,
    Layout_Vertical = 1,
} Layout_Order;

typedef struct cim_window_style
{
    // Style
    cim_vector4 Color;
    cim_u32     BorderWidth;
    cim_vector4 BorderColor;

    // Layout
    cim_vector2  Size;
    cim_vector2  Spacing;
    cim_vector4  Padding;
    Layout_Order Order;
} cim_window_style;

typedef struct cim_window
{
    cim_window_style Style;

    cim_i32 LastFrameScreenX;
    cim_i32 LastFrameScreenY;

    bool IsInitialized;
} cim_window;

typedef struct cim_button_style
{
    cim_vector4 Color;
    cim_u32     BorderWidth;
    cim_vector4 BorderColor;

    cim_vector2 Size;
} cim_button_style;

typedef struct cim_button
{
    cim_button_style Style;
} cim_button;

typedef struct cim_component
{
    bool IsInitialized;

    cim_u32  LayoutNodeIndex;
    theme_id ThemeId;
    
    CimComponent_Flag Type;
    union
    {
        cim_window Window;
        cim_button Button;
    } For;
} cim_component;

typedef struct cim_component_entry
{
    cim_component Value;
    char          Key[64];
} cim_component_entry;

typedef struct cim_component_hashmap
{
    cim_u8 *Metadata;
    cim_component_entry *Buckets;
    cim_u32              GroupCount;
    bool                 IsInitialized;
} cim_component_hashmap;

typedef struct cim_layout_node
{
    // Hierarchy
    cim_u32 Id;
    cim_u32 Parent;
    cim_u32 FirstChild;
    cim_u32 LastChild;
    cim_u32 NextSibling;

    // Layout Intent
    cim_f32 ContentWidth;
    cim_f32 ContentHeight;
    cim_f32 PrefWidth;
    cim_f32 PrefHeight;

    // Arrange Result
    cim_f32 X;
    cim_f32 Y;
    cim_f32 Width;
    cim_f32 Height;

    // Layout..
    cim_vector2  Spacing;
    cim_vector4  Padding;
    Layout_Order Order;

    // State
    bool Clicked;
    bool Held;
} cim_layout_node;

typedef struct cim_layout_tree
{
    // Memory pool
    cim_layout_node *Nodes;
    cim_u32          NodeCount;
    cim_u32          NodeSize;

    // Tree-Logic
    cim_u32 ParentStack[CimLayout_MaxNestDepth];
    cim_u32 AtParent;

    cim_u32 FirstDragNode;  // Set transforms to 0 when we pop that node.
    cim_i32 DragTransformX;
    cim_i32 DragTransformY;
} cim_layout_tree;

typedef struct cim_layout
{
    cim_layout_tree       Tree; // Forced to 1 tree for now.
    cim_component_hashmap Components;
} cim_layout;