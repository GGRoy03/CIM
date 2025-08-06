// [Headers & Includes]

#include "private/cim_private.h"

#include <stdio.h>  // TEMP: Used for reading files
#include <string.h> // memcpy, memcmp

// [Enums & Constants]

typedef enum Component_Flag
{
    Component_Invalid = 0,
    Component_Window  = 1 << 0,
} Component_Flag;

typedef enum Attribute_Type
{
    Attr_HeadColor,
    Attr_BodyColor,

    Attr_BorderColor,
    Attr_BorderWidth,

} Attribute_Type;

typedef enum Token_Type
{
    Token_String     = 255,
    Token_Identifier = 256,
    Token_Number     = 257,
    Token_Assignment = 258,
    Token_Vector     = 269,
} Token_Type;

// NOTE: Is this used? If not, why not?
typedef enum ValueFormat_Flag
{
    ValueFormat_SNumber = 1 << 0,
    ValueFormat_UNumber = 1 << 1,
    ValueFormat_Vector  = 1 << 2,
} ValueFormat_Flag;

// [Structs]

typedef struct master_style
{
    cim_vector4 HeadColor;
    cim_vector4 BodyColor;

    cim_vector4 BorderColor;
    cim_u32     BorderWidth;
    bool        HasBorders;
} master_style;

typedef struct style_desc
{
    char           Id[64];
    Component_Flag ComponentFlag;
    master_style   Style;
} style_desc;

typedef struct token
{
    Token_Type Type;

    union
    {
        cim_f32 Float32;
        cim_u32 UInt32;
        cim_i32 Int32;
        struct { cim_u8 *Name; cim_u32 NameLength; };
        cim_vector4 Vector;
    };
} token;

typedef struct lexer
{
    token  *Tokens;
    cim_u32 TokenCount;
    cim_u32 TokenCapacity;

    bool IsValid;
} lexer;

typedef struct user_styles
{
    style_desc *Descs;
    cim_u32     DescCount;
    cim_u32     DescSize;

    bool IsValid;
} user_styles;

typedef struct valid_component
{
    const char    *Name;
    size_t         Length;
    Component_Flag ComponentFlag;
} valid_component;

typedef struct valid_attribute
{
    const char      *Name;
    size_t           Length;
    Attribute_Type   Type;
    ValueFormat_Flag FormatFlags;
    Component_Flag   ComponentFlag;
} valid_attribute;

// TODO: Possibly move this out of here?
typedef struct buffer
{
    cim_u8* Data;
    cim_u32 At;
    cim_u32 Size;
} buffer;

// [Data Tables]

static valid_component ValidComponents[] =
{
    {"Window", (sizeof("Window") - 1), Component_Window},
};

static valid_attribute ValidAttributes[] =
{
    {"BodyColor", (sizeof("BodyColor") - 1), Attr_BodyColor,
    ValueFormat_Vector, Component_Window},

    {"HeadColor", (sizeof("HeadColor") - 1), Attr_HeadColor,
    ValueFormat_Vector, Component_Window},

    {"BorderColor", (sizeof("BorderColor") - 1), Attr_BorderColor,
    ValueFormat_Vector, Component_Window},

    {"BorderWidth", (sizeof("BorderWidth") - 1), Attr_BorderWidth,
    ValueFormat_UNumber, Component_Window},
};

// [Static Helper Functions]

static buffer
ReadEntireFile(const char *File)
{
    buffer Result = { 0 };

    FILE *FilePointer = fopen(File, "rb");
    if (!FilePointer) 
    {
        CimLog_Error("Failed to open the styling file | for: %s", File);
        return Result;
    }
    if (fseek(FilePointer, 0, SEEK_END) != 0)
    {
        CimLog_Error("Failed to seek the EOF | for: %s", File);
        fclose(FilePointer);
        return Result;
    }

    Result.Size = (cim_u32)ftell(FilePointer);
    if (Result.Size < 0)
    {
        CimLog_Error("Failed to tell the size of the file | for: %s", File);
        return Result;
    }

    fseek(FilePointer, 0, SEEK_SET);

    Result.Data = malloc((size_t)Result.Size);
    if (!Result.Data)
    {
        CimLog_Error("Failed to heap-allocate | for: %s, with size: %d", File, Result.Size);
        fclose(FilePointer);
        return Result;
    }

    size_t Read = fread(Result.Data, 1, (size_t)Result.Size, FilePointer);
    if (Read != (size_t)Result.Size)
    {
        CimLog_Error("Could not read the full file | for: %s", File);
        free(Result.Data);
        fclose(FilePointer);
        return Result;
    }

    fclose(FilePointer);

    return Result;
}

static inline token *
CreateToken(Token_Type Type, lexer *Lexer)
{
    if(Lexer->TokenCount == Lexer->TokenCapacity)
    {
        Lexer->TokenCapacity = Lexer->TokenCapacity ? Lexer->TokenCapacity * 2 : 64;

        token *New = malloc(Lexer->TokenCapacity * sizeof(token));
        if(!New)
        {
            CimLog_Error("Failed to heap-allocate a buffer for the tokens. OOM?");
            return NULL;
        }

        memcpy(New, Lexer->Tokens, Lexer->TokenCount * sizeof(token));
        free(Lexer->Tokens);

        Lexer->Tokens = New;
    }

    token *Token = Lexer->Tokens + Lexer->TokenCount++;
    Token->Type = Type;

    return Token;
}

static inline bool
IsAlphaCharacter(cim_u8 Character)
{
    bool Result = (Character >= 'A' && Character <= 'Z') ||
                  (Character >= 'a' && Character <= 'z');

    return Result;
}

static inline bool
IsNumberCharacter(cim_u8 Character)
{
    bool Result = (Character >= '0' && Character <= '9');

    return Result;
}

// TODO:
// 1) Track errors on hex formatting for colors

static lexer
CreateTokenStreamFromBuffer(buffer *Content)
{
    lexer Lexer = { 0 };
    Lexer.Tokens        = malloc(1000 * sizeof(token));
    Lexer.TokenCapacity = 1000;
    Lexer.IsValid       = false;

    if (!Content->Data || !Content->Size)
    {
        CimLog_Error("Cannot parse the file because the file is empty or wasn't read.");
        return Lexer;
    }

    if (!Lexer.Tokens)
    {
        CimLog_Error("Failed to heap-allocate. Out of memory?");
        return Lexer;
    }

    while (Content->At < Content->Size)
    {
        while (Content->Data[Content->At] == ' ')
        {
            Content->At += 1;
        }

        cim_u8 *Character = Content->Data + Content->At;
        cim_u32 StartAt   = Content->At;
        cim_u32 At        = StartAt;

        if (IsAlphaCharacter(*Character))
        {
            cim_u8 *Identifier = Content->Data + At;
            while (At < Content->Size && IsAlphaCharacter(*Identifier))
            {
                Identifier++;
            }

            token *Token = CreateToken(Token_Identifier, &Lexer);
            Token->Name       = Character;
            Token->NameLength = (cim_u32)(Identifier - Character);

            At += Token->NameLength;
        }
        else if (IsNumberCharacter(*Character))
        {
            token *Token = CreateToken(Token_Number, &Lexer);

            cim_u32 Number = 0;
            cim_u32 Digit  = 0;
            while(At < Content->Size && IsNumberCharacter(*Character))
            {
                Number = (Number * (10 * Digit++)) + (*Character - '0');
                ++Character;
            }

            Token->UInt32 = Number;

            At += Digit;
        }
        else if (*Character == '\r' || *Character == '\n')
        {
            //  NOTE: Doesn't do much right now, but will be used to provide better error messages.

            cim_u8 C = Content->Data[At++];
            if (C == '\r' && Content->Data[At] == '\n')
            {
                At++;
            }
        }
        else if (*Character == ':')
        {
            At++;

            if (At < Content->Size && Content->Data[At] == '=')
            {
                CreateToken(Token_Assignment, &Lexer);
                At++;
            }
            else
            {
                CimLog_Error("Stray ':' token. Did you mean := (Assignment)?");
                return Lexer;
            }
        }
        else if (*Character == '"')
        {
            At++;

            token *Token = CreateToken(Token_String, &Lexer);
            Token->Name = Content->Data + At;

            while (IsAlphaCharacter(Content->Data[At]))
            {
                At++;
            }

            if (Content->Data[At] != '"')
            {
                CimLog_Error("...");
                return Lexer;
            }

            Token->NameLength = (cim_u32)((Content->Data + At++) - Token->Name);
        }
        else if (*Character == '#')
        {
            At++;

            cim_u32 MaximumHex = 8; // (#RRGGBBAA)
            cim_u32 HexCount   = 0;
            cim_u32 UValue     = 0;

            while (HexCount < MaximumHex)
            {
                char    C     = Content->Data[At + HexCount];
                cim_u32 Digit = 0;

                if      (C >= '0' && C <= '9') Digit = C - '0';
                else if (C >= 'A' && C <= 'F') Digit = C - 'A' + 10;
                else if (C >= 'a' && C <= 'f') Digit = C - 'a' + 10;
                else break;

                UValue = (UValue << 4) | Digit;

                HexCount++;
            }

            At += HexCount;

            if (IsNumberCharacter(Content->Data[At]) || IsAlphaCharacter(Content->Data[At]))
            {
                CimLog_Error("...");
                return Lexer;
            }

            cim_f32 Inverse = 1.0f / 255.0f;
            token  *Token   = CreateToken(Token_Vector, &Lexer);
            Token->Vector.x = (cim_f32)((UValue >> ((HexCount == 6) ? 16 : 24)) & 0xFF) * Inverse;
            Token->Vector.y = (cim_f32)((UValue >> ((HexCount == 6) ? 8  : 16)) & 0xFF) * Inverse;
            Token->Vector.z = (cim_f32)((UValue >> ((HexCount == 6) ? 0  : 8)) & 0xFF) * Inverse;
            Token->Vector.w = (HexCount == 8) ? (cim_f32)(UValue & 0xFF) * Inverse : 1.0f;
        }
        else 
        {
            // WARN: Still unsure.
            CreateToken(*Character, &Lexer);
            At++;
        }

        Content->At = At;
    }

    Lexer.IsValid= true;
    return Lexer;
}

// TODO: 
// 1) Improve the error messages & Continue implementing the logic.

static user_styles
CreateUserStyles(lexer *Lexer)
{
    user_styles Styles = { 0 };
    Styles.Descs    = calloc(10, sizeof(style_desc));
    Styles.DescSize = 100;
    Styles.IsValid  = false;

    cim_u32 AtToken = 0;
    while (AtToken < Lexer->TokenCount)
    {
        token *Token = Lexer->Tokens + AtToken;
        AtToken     += 1;

        switch (Token->Type)
        {

        case Token_String:
        {
            token *Next = Lexer->Tokens + AtToken;
            AtToken    += 1;

            if (Next->Type != Token_Assignment)
            {
                CimLog_Error("...");
                return Styles;
            }

            Next    = Lexer->Tokens + AtToken;
            AtToken += 1;

            if (Next->Type != Token_Identifier)
            {
                CimLog_Error("...");
                return Styles;
            }

            bool IsValidComponentName = false;
            for (cim_u32 ValidIdx = 0; ValidIdx < CIM_ARRAY_COUNT(ValidComponents); ValidIdx++)
            {
                valid_component *Valid = ValidComponents + ValidIdx;

                if (Valid->Length != Next->NameLength)
                {
                    continue;
                }

                if (memcmp(Next->Name, Valid->Name, Next->NameLength) == 0)
                {
                    // BUG: Does not check for overflows.
                    style_desc *Desc = Styles.Descs + Styles.DescCount++;

                    memcpy(Desc->Id, Token->Name, Token->NameLength);
                    Desc->ComponentFlag = Valid->ComponentFlag;

                    IsValidComponentName = true;

                    break;
                }
            }

            if (!IsValidComponentName)
            {
                CimLog_Error("...");
                return Styles;
            }
        } break;

        case Token_Identifier:
        {
            valid_attribute Attr  = { 0 };
            bool            Found = false;

            style_desc *Desc = Styles.Descs + (Styles.DescCount - 1);
            if (Styles.DescCount == 0 || Desc->ComponentFlag == Component_Invalid && !Desc)
            {
                CimLog_Error("...");
                return Styles;
            }

            for (cim_u32 Idx = 0; Idx < CIM_ARRAY_COUNT(ValidAttributes); Idx++)
            {
                valid_attribute Valid = ValidAttributes[Idx];

                if (Valid.Length != Token->NameLength || !(Valid.ComponentFlag & Desc->ComponentFlag))
                {
                    continue;
                }

                if (memcmp(Valid.Name, Token->Name, Token->NameLength) == 0)
                {
                    Attr  = Valid;
                    Found = true;

                    break;
                }
            }

            if (!Found)
            {
                CimLog_Error("...");
                return Styles;
            }

            token *Next = Lexer->Tokens + AtToken;
            AtToken += 1;

            if (Next->Type != Token_Assignment)
            {
                CimLog_Error("...");
                return Styles;
            }

            Next     = Lexer->Tokens + AtToken;
            AtToken += 1;

            bool IsNegative = false;
            if(Next->Type == '-')
            {
                IsNegative = true;

                Next     = Lexer->Tokens + AtToken;
                AtToken += 1;
            }

            switch (Attr.Type)
            {

            case Attr_BodyColor:
            {
                Desc->Style.BodyColor = Next->Vector;
            } break;

            case Attr_HeadColor:
            {
                Desc->Style.HeadColor = Next->Vector;
            } break;

            case Attr_BorderColor:
            {
                Desc->Style.BorderColor = Next->Vector;
            } break;

            case Attr_BorderWidth:
            {
                Desc->Style.BorderWidth = Next->UInt32;
            } break;

            default:
            {
                CimLog_Error("...");
                return Styles;
            } break;

            }

            Next = Lexer->Tokens + AtToken;
            AtToken += 1;

            if (Next->Type != ';')
            {
                CimLog_Error("...");
                return Styles;
            }

        } break;

        default:
        {
            CimLog_Info("Skipping unknown token.");
        } break;

        }

    }

    Styles.IsValid = true;
    return Styles;
}

static inline bool
SetUserStyles(user_styles *Styles)
{
    for(cim_u32 DescIdx = 0; DescIdx < Styles->DescCount; DescIdx++)
    {
        style_desc    *Desc      = Styles->Descs + DescIdx;
        cim_component *Component = CimComponent_GetOrInsert(Desc->Id, &CimContext->ComponentStore);

        switch (Desc->ComponentFlag)
        {

        case Component_Window:
        {
            cim_window *Window = &Component->For.Window;

            Window->Style.BodyColor = Desc->Style.BodyColor;
            Window->Style.HeadColor = Desc->Style.HeadColor;

            Window->Style.HasBorders  = true;
            Window->Style.BorderColor = Desc->Style.BorderColor;
            Window->Style.BorderWidth = Desc->Style.BorderWidth;

            // WARN: Force set this for now. When we rewrite the hot-reload
            // we need to fix this.
            Component->StyleUpdateFlags |= StyleUpdate_BorderGeometry;
        } break;

        }
    }

    return true;
}

// [API Implementation]

#ifdef __cplusplus
extern "C" {
#endif

void 
CimStyle_Initialize(const char *File)
{
    buffer FileContent = ReadEntireFile(File);
    if (!FileContent.Data)
    {
        CimLog_Fatal("Failed at: Reading file. See Error(s) Above");
        return;
    }

    lexer Lexer = CreateTokenStreamFromBuffer(&FileContent);
    if (!Lexer.IsValid)
    {
        CimLog_Fatal("Failed at: Creating token stream. See Error(s) Above");
        return;
    }

    user_styles Styles = CreateUserStyles(&Lexer);
    if(!Styles.IsValid)
    {
        CimLog_Fatal("Failed at: Creating user styles. See Error(s) Above");
        return;
    }

    if (!SetUserStyles(&Styles))
    {
        CimLog_Fatal("Failed at: Setting user styles. See Error(s) Above.");
        return;
    }

    free(FileContent.Data);
    free(Lexer.Tokens);
    free(Styles.Descs);
}

#ifdef __cplusplus
}
#endif
