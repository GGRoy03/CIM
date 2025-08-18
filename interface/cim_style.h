#pragma once

typedef enum ThemeAttribute_Flag
{
    ThemeAttribute_None        = 0,
    ThemeAttribute_Size        = 1 << 0,
    ThemeAttribute_Color       = 1 << 1,
    ThemeAttribute_Padding     = 1 << 2,
    ThemeAttribute_Spacing     = 1 << 3,
    ThemeAttribute_LayoutOrder = 1 << 4,
    ThemeAttribute_BorderColor = 1 << 5,
    ThemeAttribute_BorderWidth = 1 << 6,
} ThemeAttribute_Flag;

typedef enum ThemeToken_Type
{
    ThemeToken_String     = 1 << 8,
    ThemeToken_Identifier = 1 << 9,
    ThemeToken_Number     = 1 << 10,
    ThemeToken_Assignment = 1 << 11,
    ThemeToken_Vector     = 1 << 12,
    ThemeToken_Theme      = 1 << 13,
    ThemeToken_For        = 1 << 14,
} ThmeToken_Type;

typedef enum ThemeParsing_State
{
    ThemeParsing_None   = 0,
    ThemeParsing_Window = 1,
    ThemeParsing_Button = 2,
    ThemeParsing_Count  = 3,
} ThemeParsing_State;

typedef enum ThemeParsingError_Type
{
    ThemeParsingError_None     = 0,
    ThemeParsingError_Syntax   = 1,
    ThemeParsingError_Argument = 2,
    ThemeParsingError_Internal = 3,
} ThemeParsingError_Type;

typedef struct theme_parsing_error
{
    ThemeParsingError_Type Type;
    char                   Message[256];

    union
    {
        cim_u32 LineInFile;
        cim_u32 ArgumentIndex;
    };
} theme_parsing_error;

typedef struct theme_token
{
    cim_u32 LineInFile;

    ThemeToken_Type Type;
    union
    {
        cim_f32 Float32;
        cim_u32 UInt32;
        struct
        {
            cim_u8 *At;
            cim_u32 Size;
        } Identifier;
        struct
        {
            cim_u32 DataU32[4];
            cim_u32 Size;
        } Vector;
    };
} theme_token;

typedef struct window_theme
{
    cim_vector4 Color;
    cim_vector4 BorderColor;
    cim_u32     BorderWidth;

    cim_vector2 Size;
    cim_vector2 Spacing;
    cim_vector4 Padding;
} cim_window_theme;

typedef struct button_theme
{
    cim_vector4 Color;
    cim_vector4 BorderColor;
    cim_u32     BorderWidth;
} cim_button_theme;

typedef struct theme_parser
{
    buffer TokenBuffer;

    cim_u32 AtLine;
    cim_u32 AtToken;

    ThemeParsing_State State;
    union
    {
        window_theme Window;
        button_theme Button;
    } ActiveTheme;

} theme_parser;