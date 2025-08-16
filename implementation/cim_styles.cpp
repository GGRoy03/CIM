// WARN: File is a bit messy.

#define FAIL_ON_NEGATIVE(Negative, Message, ...) if(Negative) {CimLog_Error(Message, __VA_ARGS__);}
#define ARRAY_TO_VECTOR2(Array, Length, Vector)  if(Length > 0) Vector.x = Array[0]; if(Length > 1) Vector.y = Array[1];
#define ARRAY_TO_VECTOR4(Array, Length, Vector)  if(Length > 0) Vector.x = Array[0]; if(Length > 1) Vector.y = Array[1]; \
                                                 if(Length > 2) Vector.z = Array[2]; if(Length > 3) Vector.w = Array[3];

typedef enum Attribute_Type
{
    Attribute_Size        = 0,
    Attribute_Color       = 1,
    Attribute_Padding     = 2,
    Attribute_Spacing     = 3,
    Attribute_LayoutOrder = 4,
    Attribute_BorderColor = 5,
    Attribute_BorderWidth = 6,
} Attribute_Type;

typedef enum Token_Type
{
    Token_String     = 255,
    Token_Identifier = 256,
    Token_Number     = 257,
    Token_Assignment = 258,
    Token_Vector     = 269,
} Token_Type;

typedef struct master_style
{
    // Styling
    cim_vector4 Color;
    cim_vector4 BorderColor;
    cim_u32     BorderWidth;

    // Layout/Positioning
    cim_vector2  Size;
    cim_vector2  Spacing;
    cim_vector4  Padding;
    Layout_Order Order;
} master_style;

typedef struct style_desc
{
    char              Id[64];
    CimComponent_Flag ComponentFlag;
    master_style      Style;
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
    token *Tokens;
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
    const char *Name;
    size_t            Length;
    CimComponent_Flag ComponentFlag;
} valid_component;

typedef struct valid_attribute
{
    const char *Name;
    size_t         Length;
    Attribute_Type Type;
    cim_bit_field  ComponentFlag;
} valid_attribute;

valid_component ValidComponents[] =
{
    {"Window", (sizeof("Window") - 1), CimComponent_Window},
    {"Button", (sizeof("Button") - 1), CimComponent_Button},
};

valid_attribute ValidAttributes[] =
{
    {"Size", (sizeof("Size") - 1), Attribute_Size,
     CimComponent_Window | CimComponent_Button},

    {"Color", (sizeof("Color") - 1), Attribute_Color,
     CimComponent_Window | CimComponent_Button},

    {"Padding", (sizeof("Padding") - 1), Attribute_Padding,
     CimComponent_Window},

    {"Spacing", (sizeof("Spacing") - 1), Attribute_Spacing,
     CimComponent_Window},

    {"LayoutOrder", (sizeof("LayoutOrder") - 1), Attribute_LayoutOrder,
     CimComponent_Window},

    {"BorderColor", (sizeof("BorderColor") - 1), Attribute_BorderColor,
     CimComponent_Window | CimComponent_Button},

    {"BorderWidth", (sizeof("BorderWidth") - 1), Attribute_BorderWidth,
     CimComponent_Window | CimComponent_Button},
};

buffer
ReadEntireFile(const char *File)
{
    buffer Result = {};

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

    Result.Data = (cim_u8 *)malloc((size_t)Result.Size);
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

token *
CreateToken(Token_Type Type, lexer *Lexer)
{
    if (Lexer->TokenCount == Lexer->TokenCapacity)
    {
        Lexer->TokenCapacity = Lexer->TokenCapacity ? Lexer->TokenCapacity * 2 : 64;

        token *New = (token *)malloc(Lexer->TokenCapacity * sizeof(token));
        if (!New)
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

bool
IsAlphaCharacter(cim_u8 Character)
{
    bool Result = (Character >= 'A' && Character <= 'Z') ||
        (Character >= 'a' && Character <= 'z');

    return Result;
}

bool
IsNumberCharacter(cim_u8 Character)
{
    bool Result = (Character >= '0' && Character <= '9');

    return Result;
}

lexer
CreateTokenStreamFromBuffer(buffer *Content)
{
    lexer Lexer = {};
    Lexer.Tokens = (token *)malloc(1000 * sizeof(token));
    Lexer.TokenCapacity = 1000;
    Lexer.IsValid = false;

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
        cim_u32 StartAt = Content->At;
        cim_u32 At = StartAt;

        if (IsAlphaCharacter(*Character))
        {
            cim_u8 *Identifier = Content->Data + At;
            while (At < Content->Size && IsAlphaCharacter(*Identifier))
            {
                Identifier++;
            }

            token *Token = CreateToken(Token_Identifier, &Lexer);
            Token->Name = Character;
            Token->NameLength = (cim_u32)(Identifier - Character);

            At += Token->NameLength;
        }
        else if (IsNumberCharacter(*Character))
        {
            cim_u32 Number = 0;
            while (At < Content->Size && IsNumberCharacter(Content->Data[At]))
            {
                Number = (Number * 10) + (*Character - '0');
                At += 1;
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
            // NOTE: The formatting rules are quite strict. And weird regarding
            // whitespaces.

            At++;

            cim_f32 Vector[4] = { 0 };
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

            cim_f32 Vector[4] = { 0.0f };
            cim_i32 VectorIdx = 0;
            cim_f32 Inverse = 1.0f / 255.0f;
            cim_u32 MaximumHex = 8; // (#RRGGBBAA)
            cim_u32 HexCount = 0;

            while ((HexCount + 1) < MaximumHex && VectorIdx < 4)
            {
                cim_u32 Value = 0;
                bool    Valid = true;
                for (cim_u32 Idx = 0; Idx < 2; Idx++)
                {
                    char    C = Content->Data[At + HexCount + Idx];
                    cim_u32 Digit = 0;

                    if (C >= '0' && C <= '9') Digit = C - '0';
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
            CreateToken((Token_Type)*Character, &Lexer);
            At++;
        }

        Content->At = At;
    }

    Lexer.IsValid = true;
    return Lexer;
}

// TODO: 
// 1) Improve the error messages & Continue implementing the logic.

user_styles
CreateUserStyles(lexer *Lexer)
{
    user_styles Styles = {};
    Styles.Descs = (style_desc *)calloc(10, sizeof(style_desc));
    Styles.DescSize = 100;
    Styles.IsValid = false;

    cim_u32 AtToken = 0;
    while (AtToken < Lexer->TokenCount)
    {
        token *Token = Lexer->Tokens + AtToken;
        AtToken += 1;

        switch (Token->Type)
        {

        case Token_String:
        {
            token *Next = Lexer->Tokens + AtToken;
            AtToken += 1;

            if (Next->Type != Token_Assignment)
            {
                CimLog_Error("...");
                return Styles;
            }

            Next = Lexer->Tokens + AtToken;
            AtToken += 1;

            if (Next->Type != Token_Identifier)
            {
                CimLog_Error("...");
                return Styles;
            }

            bool IsValidComponentName = false;
            for (cim_u32 ValidIdx = 0; ValidIdx < Cim_ArrayCount(ValidComponents); ValidIdx++)
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
            valid_attribute Attr = {};
            bool            Found = false;

            style_desc *Desc = Styles.Descs + (Styles.DescCount - 1);
            if (Styles.DescCount == 0 || Desc->ComponentFlag == CimComponent_Invalid || !Desc)
            {
                CimLog_Error("...");
                return Styles;
            }

            for (cim_u32 Idx = 0; Idx < Cim_ArrayCount(ValidAttributes); Idx++)
            {
                valid_attribute Valid = ValidAttributes[Idx];

                if (Valid.Length != Token->NameLength || !(Valid.ComponentFlag & Desc->ComponentFlag))
                {
                    continue;
                }

                if (memcmp(Valid.Name, Token->Name, Token->NameLength) == 0)
                {
                    Attr = Valid;
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

            Next = Lexer->Tokens + AtToken;
            AtToken += 1;

            bool IsNegative = false;
            if (Next->Type == '-')
            {
                IsNegative = true;

                Next = Lexer->Tokens + AtToken;
                AtToken += 1;
            }

            // NOTE: Can I not compress this? Structure seems a bit bad?
            // Something is weird with this data flow.

            switch (Attr.Type)
            {

            case Attribute_Size:
            {
                FAIL_ON_NEGATIVE(IsNegative, "Value cannot be negative.");
                ARRAY_TO_VECTOR2(Next->Vector, Next->VectorSize, Desc->Style.Size)
            } break;

            case Attribute_Color:
            {
                FAIL_ON_NEGATIVE(IsNegative, "Value cannot be negative.");
                ARRAY_TO_VECTOR4(Next->Vector, Next->VectorSize, Desc->Style.Color)
            } break;

            case Attribute_Spacing:
            {
                FAIL_ON_NEGATIVE(IsNegative, "Value cannot be negative.");
                ARRAY_TO_VECTOR2(Next->Vector, Next->VectorSize, Desc->Style.Spacing)
            } break;

            case Attribute_Padding:
            {
                FAIL_ON_NEGATIVE(IsNegative, "Value cannot be negative.");
                ARRAY_TO_VECTOR4(Next->Vector, Next->VectorSize, Desc->Style.Padding)
            } break;

            case Attribute_LayoutOrder:
            {
                FAIL_ON_NEGATIVE(IsNegative, "Value cannot be negative.");
                if (Next->NameLength == 10) // Hack because lazy
                {
                    Desc->Style.Order = Layout_Horizontal;
                }
                else
                {
                    Desc->Style.Order = Layout_Vertical;
                }
            } break;

            case Attribute_BorderColor:
            {
                FAIL_ON_NEGATIVE(IsNegative, "Value cannot be negative.");
                ARRAY_TO_VECTOR4(Next->Vector, Next->VectorSize, Desc->Style.BorderColor)
            } break;

            case Attribute_BorderWidth:
            {
                FAIL_ON_NEGATIVE(IsNegative, "Value cannot be negative.");
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

bool
CimStyle_Set(user_styles *Styles)
{
    for (cim_u32 DescIdx = 0; DescIdx < Styles->DescCount; DescIdx++)
    {
        style_desc *Desc = Styles->Descs + DescIdx;
        cim_component *Component = FindComponent(Desc->Id);

        switch (Desc->ComponentFlag)
        {

        case CimComponent_Window:
        {
            cim_window *Window = &Component->For.Window;

            // Style
            Window->Style.Color = Desc->Style.Color;
            Window->Style.BorderColor = Desc->Style.BorderColor;
            Window->Style.BorderWidth = Desc->Style.BorderWidth;

            // Layout
            Window->Style.Size = Desc->Style.Size;
            Window->Style.Order = Desc->Style.Order;
            Window->Style.Padding = Desc->Style.Padding;
            Window->Style.Spacing = Desc->Style.Spacing;
        } break;

        case CimComponent_Button:
        {
            cim_button *Button = &Component->For.Button;

            // Style
            Button->Style.Color = Desc->Style.Color;
            Button->Style.BorderColor = Desc->Style.BorderColor;
            Button->Style.BorderWidth = Desc->Style.BorderWidth;

            // Layout
            Button->Style.Size = Desc->Style.Size;
        } break;

        case CimComponent_Invalid:
        {
            CimLog_Error("...");
            return false;
        }

        }
    }

    return true;
}

static void
InitializeStyle(const char *File)
{
    buffer      FileContent = {};
    lexer       Lexer = {};
    user_styles Styles = {};

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
    if (!Styles.IsValid)
    {
        goto Cleanup;
        CimLog_Fatal("Failed at: Creating user styles. See Error(s) Above");
        return;
    }

    if (!CimStyle_Set(&Styles))
    {
        goto Cleanup;
        CimLog_Fatal("Failed at: Setting user styles. See Error(s) Above.");
        return;
    }

Cleanup:
    if (FileContent.Data) free(FileContent.Data);
    if (Lexer.Tokens)     free(Lexer.Tokens);
    if (Styles.Descs)     free(Styles.Descs);
}