// [Headers & Includes]

#include "private/cim_private.h"

#include <stdio.h>  // TEMP: Used for reading files
#include <string.h> // memcpy, memcmp

// [Enums & Constants & Macros]

#define FAIL_ON_NEGATIVE(Negative, Message, ...) if(Negative) {CimLog_Error(Message, __VA_ARGS__);}
#define ARRAY_TO_VECTOR2(Array, Length, Vector) if(Length > 0) Vector.x = Array[0]; if(Length > 1) Vector.y = Array[1];
#define ARRAY_TO_VECTOR4(Array, Length, Vector) if(Length > 0) Vector.x = Array[0]; if(Length > 1) Vector.y = Array[1]; \
                                                if(Length > 2) Vector.z = Array[2]; if(Length > 3) Vector.w = Array[3];

typedef enum Attribute_Type
{
    Attr_HeadColor,
    Attr_BodyColor,

    Attr_BorderColor,
    Attr_BorderWidth,

    Attr_Position,
    Attr_Dimension,

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

    cim_vector2 Position;
    cim_vector2 Dimension;
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
        struct { cim_f32 Vector[4]; cim_u32 VectorSize; };
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
    {"Button", (sizeof("Button") - 1), Component_Button},
};

static valid_attribute ValidAttributes[] =
{
    {"BodyColor", (sizeof("BodyColor") - 1), Attr_BodyColor,
    ValueFormat_Vector, Component_Window | Component_Button},

    {"HeadColor", (sizeof("HeadColor") - 1), Attr_HeadColor,
    ValueFormat_Vector, Component_Window},

    {"BorderColor", (sizeof("BorderColor") - 1), Attr_BorderColor,
    ValueFormat_Vector, Component_Window | Component_Button},

    {"BorderWidth", (sizeof("BorderWidth") - 1), Attr_BorderWidth,
    ValueFormat_UNumber, Component_Window | Component_Button},

    {"Position", (sizeof("Position") - 1), Attr_Position,
    ValueFormat_Vector, Component_Window | Component_Button},

    {"Dimension", (sizeof("Dimension") - 1), Attr_Dimension,
    ValueFormat_Vector, Component_Window | Component_Button},
};

// [Static Helper Functions]

static buffer
ReadEntireFile(const char *File)
{
    buffer Result = { 0 };

    FILE *FilePointer;
    fopen_s(&FilePointer, File, "rb");
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

        free(Result.Data);
        Result.Data = NULL;

        fclose(FilePointer);

        return Result;
    }

    size_t Read = fread(Result.Data, 1, (size_t)Result.Size, FilePointer);
    if (Read != (size_t)Result.Size)
    {
        CimLog_Error("Could not read the full file | for: %s", File);

        free(Result.Data);
        Result.Data = NULL;

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

static lexer
CreateTokenStreamFromBuffer(buffer *Content)
{
    lexer Lexer = { 0 };
    Lexer.Tokens        = malloc(1000 * sizeof(token));
    Lexer.TokenCapacity = 1000;
    Lexer.IsValid       = false;

    if (!Content->Data || !Content->Size)
    {
        CimLog_Error("Error parsing file: The file is empty or wasn't read.");
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
            cim_u32 Number = 0;
            while(At < Content->Size && IsNumberCharacter(Content->Data[At]))
            {
                Number = (Number * 10) + (*Character - '0');
                At    += 1;
            }

            token *Token = CreateToken(Token_Number, &Lexer);
            Token->UInt32 = Number;
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
        else if (*Character == '[')
        { 
            // NOTE: The formatting rules are quite strict.

            At++;

            cim_f32 Vector[4]  = {0};
            cim_u32 DigitCount = 0;
            while (DigitCount < 4 && Content->Data[At] != ';')
            {
                while (At < Content->Size && Content->Data[At] == ' ')
                {
                    At++;
                }

                cim_u32 Number = 0;
                while (At < Content->Size && IsNumberCharacter(Content->Data[At]))
                {
                    Number = (Number * 10) + (Content->Data[At] - '0');
                    At += 1;
                }

                if (Content->Data[At] != ',' && Content->Data[At] != ']')
                {
                    CimLog_Error("...");
                    return Lexer;
                }

                At++;
                Vector[DigitCount++] = Number;
            }

            token *Token = CreateToken(Token_Vector, &Lexer);
            memcpy(Token->Vector, Vector, sizeof(Token->Vector));
            Token->VectorSize = DigitCount;
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

            cim_f32 Vector[4]  = { 0.0f };
            cim_i32 VectorIdx  = 0;
            cim_f32 Inverse    = 1.0f / 255.0f;
            cim_u32 MaximumHex = 8; // (#RRGGBBAA)
            cim_u32 HexCount   = 0;

            while ((HexCount + 1) < MaximumHex && VectorIdx < 4)
            {
                cim_u32 Value = 0;
                bool    Valid = true;
                for (cim_u32 Idx = 0; Idx < 2; Idx++)
                {
                    char    C     = Content->Data[At + HexCount + Idx];
                    cim_u32 Digit = 0;

                    if      (C >= '0' && C <= '9') Digit = C - '0';
                    else if (C >= 'A' && C <= 'F') Digit = C - 'A' + 10;
                    else if (C >= 'a' && C <= 'f') Digit = C - 'a' + 10;
                    else { Valid = false; break; }

                    Value = (Value << 4) | Digit;
                }

                if (!Valid) break;

                Vector[VectorIdx++] = Value * Inverse;
                HexCount += 2;
            }

            At += HexCount;

            if (IsNumberCharacter(Content->Data[At]) || IsAlphaCharacter(Content->Data[At]))
            {
                CimLog_Error("...");
                return Lexer;
            }

            if (HexCount == 6)
            {
                Vector[3] = 1.0f;
            }

            token *Token = CreateToken(Token_Vector, &Lexer);
            memcpy(Token->Vector, Vector, sizeof(Token->Vector));
            Token->VectorSize = 4;
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
            if (Styles.DescCount == 0 || Desc->ComponentFlag == Component_Invalid || !Desc)
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
                FAIL_ON_NEGATIVE(IsNegative, "Value cannot be negative.");
                ARRAY_TO_VECTOR4(Next->Vector, Next->VectorSize, Desc->Style.BodyColor)
            } break;

            case Attr_HeadColor:
            {
                FAIL_ON_NEGATIVE(IsNegative, "Value cannot be negative.");
                ARRAY_TO_VECTOR4(Next->Vector, Next->VectorSize, Desc->Style.HeadColor)
            } break;

            case Attr_BorderColor:
            {
                FAIL_ON_NEGATIVE(IsNegative, "Value cannot be negative.");
                ARRAY_TO_VECTOR4(Next->Vector, Next->VectorSize, Desc->Style.BorderColor)
            } break;

            case Attr_BorderWidth:
            {
                FAIL_ON_NEGATIVE(IsNegative, "Value cannot be negative.");
                Desc->Style.BorderWidth = Next->UInt32;
            } break;

            case Attr_Position:
            {
                FAIL_ON_NEGATIVE(IsNegative, "Value cannot be negative");
                ARRAY_TO_VECTOR2(Next->Vector, Next->VectorSize, Desc->Style.Position)
            } break;

            case Attr_Dimension:
            {
                FAIL_ON_NEGATIVE(IsNegative, "Value cannot be negative");
                ARRAY_TO_VECTOR2(Next->Vector, Next->VectorSize, Desc->Style.Dimension)
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
        style_desc *Desc = Styles->Descs + DescIdx;

        cim_bit_field Flags = QueryComponent_AvoidHierarchy;
        Flags |= Desc->ComponentFlag & Component_Window ? QueryComponent_IsTreeRoot : 0;

        component *Component = QueryComponent(Desc->Id, Flags);

        switch (Desc->ComponentFlag)
        {

        case Component_Window:
        {
            window *Window = &Component->For.Window;

            Window->Style.BodyColor = Desc->Style.BodyColor;
            Window->Style.HeadColor = Desc->Style.HeadColor;

            Window->Style.HasBorders  = true;
            Window->Style.BorderColor = Desc->Style.BorderColor;
            Window->Style.BorderWidth = Desc->Style.BorderWidth;

            // WARN: Force set this for now. When we rewrite the hot-reload
            // we need to fix this.
            Component->StyleUpdateFlags |= StyleUpdate_BorderGeometry;
        } break;

        case Component_Button:
        {
            button *Button = &Component->For.Button;

            Button->Style.BodyColor = Desc->Style.BodyColor;

            Button->Style.Position  = Desc->Style.Position;
            Button->Style.Dimension = Desc->Style.Dimension;
        } break;

        case Component_Invalid:
        {
            CimLog_Error("...");
            return false;
        }

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
    buffer      FileContent = { 0 };
    lexer       Lexer       = { 0 };
    user_styles Styles      = { 0 };

    FileContent = ReadEntireFile(File);
    if (!FileContent.Data)
    {
        CimLog_Fatal("Failed at: Reading file. See Error(s) Above");
        return;
    }

    Lexer = CreateTokenStreamFromBuffer(&FileContent);
    if (!Lexer.IsValid)
    {
        goto Cleanup;
        CimLog_Fatal("Failed at: Creating token stream. See Error(s) Above");
        return;
    }

    Styles = CreateUserStyles(&Lexer);
    if(!Styles.IsValid)
    {
        goto Cleanup;
        CimLog_Fatal("Failed at: Creating user styles. See Error(s) Above");
        return;
    }

    if (!SetUserStyles(&Styles))
    {
        goto Cleanup;
        CimLog_Fatal("Failed at: Setting user styles. See Error(s) Above.");
        return;
    }

Cleanup:
    if(FileContent.Data) free(FileContent.Data);
    if(Lexer.Tokens)     free(Lexer.Tokens);
    if(Styles.Descs)     free(Styles.Descs);
}

#ifdef __cplusplus
}
#endif
