// NOTE:
// 1) I don't think we want to define the API here. Put all of the 'real' static
//    files at the top. Probably just put the function that should be called by other
//    pieces of code at the top.
// 2) The parsing seems to work fine. And the code seems better at first glance than what it was.
//     150 lines shorter, with clearer paths. But the main problem persist with the "IO thread"
//     do we want that?
// 3)  The errors are still garbage. We need to report the line and tile name. Fuck the columns
//     I think. Just use the line and the file + wayyyyy clear error messages.
// 4)  Find a way to do the hot-reload here. Like query the platform for updates and fully reparse
//     the file should be good enough, until it isn't? Means the platform thing must be async...

// [Public API]
static void InitializeUIThemes(char **Files, cim_u32 FileCount);

// [Internal]
static theme_token  *CreateThemeToken   (ThemeToken_Type Type, cim_u32 Line, cim_u32 Col, buffer *TokenBuffer);
static void          IgnoreWhiteSpaces  (buffer *Content);
static buffer        TokenizeThemeFile  (char *FileName);
static bool          StoreAttribute     (ThemeAttribute_Flag Attribute, theme_token *Value, attribute_parser *Parser);

#define IsAlphaCharacter(C)  ((C >= 'A' && C <= 'Z') || (C >= 'a' && C <= 'z'))
#define IsNumberCharacter(C) (C >= '0' && C <= '9')

#define CimTheme_MaxVectorSize 4
#define CimTheme_ArrayToVec2(Array) {Array[0], Array[1]}
#define CimTheme_ArrayToVec4(Array) {Array[0], Array[1], Array[2], Array[3]}

static theme_token *
CreateThemeToken(ThemeToken_Type Type, cim_u32 Line, cim_u32 Col, buffer *TokenBuffer)
{
    if (!IsValidBuffer(TokenBuffer))
    {
        size_t NewSize = (TokenBuffer->Size * 2) * sizeof(theme_token);
        buffer Temp    = AllocateBuffer(NewSize);
        Temp.At = TokenBuffer->At;

        memcpy(Temp.Data, TokenBuffer->Data, TokenBuffer->Size);

        FreeBuffer(TokenBuffer);

        TokenBuffer->Data = Temp.Data;
        TokenBuffer->Size = NewSize;
        TokenBuffer->At   = Temp.At;
    }

    theme_token *Result = (theme_token*)(TokenBuffer->Data + TokenBuffer->At);
    Result->Line = Line;
    Result->Col  = Col;
    Result->Type = Type;

    TokenBuffer->At += sizeof(theme_token);

    return Result;
}

static void
IgnoreWhiteSpaces(buffer *Content)
{
    while(IsValidBuffer(Content) && Content->Data[Content->At] == ' ')
    {
        Content->At++;
    }
}

static cim_u8
ToLowerChar(cim_u8 Char)
{
    if (Char >= 'A' && Char <= 'Z') Char += ('a' - 'A');

    return Char;
}

static buffer
TokenizeThemeFile(char *FileName)
{
    buffer Tokens      = {};
    buffer FileContent = ReadEntireFile(FileName);

    if(!FileContent.Data)
    {
        CimLog_Error("Unable to read theme file: %s", FileName);
        return Tokens;
    }

    if(!FileContent.Size)
    {
        CimLog_Warn("Them file %s is empty", FileName);
        return Tokens;
    }

    cim_u32 TokenCountEstimate = FileContent.Size / 10;
    size_t  TokenSizeEstimate  = TokenCountEstimate * sizeof(theme_token);
    Tokens = AllocateBuffer(TokenSizeEstimate);

    cim_u32 LineInFile = 0;
    cim_u32 ColInFile  = 0;
    while(IsValidBuffer(&FileContent))
    {
        IgnoreWhiteSpaces(&FileContent);

        cim_u8  Char    = FileContent.Data[FileContent.At];
        cim_u64 At      = FileContent.At;
        cim_u64 StartAt = At;

        switch(Char)
        {

        case 'A' ... 'Z':
        case 'a' ... 'z':
        {
            At++;

            Char = FileContent.Data[At];
            while (IsValidBuffer(&FileContent) && IsAlphaCharacter(Char))
            {
                Char = FileContent.Data[++At];
            }

            cim_u32 IdLength = At - StartAt;
            cim_u8 *IdPtr    = FileContent.Data + StartAt;

            if (IdLength == 5                &&
                ToLowerChar(IdPtr[0]) == 't' &&
                ToLowerChar(IdPtr[1]) == 'h' &&
                ToLowerChar(IdPtr[2]) == 'e' &&
                ToLowerChar(IdPtr[3]) == 'm' &&
                ToLowerChar(IdPtr[4]) == 'e')
            {
                CreateThemeToken(ThemeToken_Theme, LineInFile, ColInFile, &Tokens);
            }
            else if (IdLength == 3                &&
                     ToLowerChar(IdPtr[0]) == 'f' &&
                     ToLowerChar(IdPtr[1]) == 'o' &&
                     ToLowerChar(IdPtr[2]) == 'r')
            {
                CreateThemeToken(ThemeToken_For, LineInFile, ColInFile, &Tokens);
            }
            else
            {
                theme_token *Token = CreateThemeToken(ThemeToken_Identifier, LineInFile, ColInFile, &Tokens);
                Token->Identifier.At   = IdPtr;
                Token->Identifier.Size = IdLength;
            }
        } break;

        case '0' ... '9':
        {
            theme_token *Token = CreateThemeToken(ThemeToken_Number, LineInFile, ColInFile, &Tokens);
            Token->UInt32 = Char - '0';

            Char = FileContent.Data[++At];
            while (IsValidBuffer(&FileContent) && IsNumberCharacter(Char))
            {
                Token->UInt32 = (Token->UInt32 * 10) + (Char - '0');
                Char          = FileContent.Data[At++];
            }
        } break;

        case '\r':
        case '\n':
        {
            At++;
            if (Char == '\r' && FileContent.Data[At] == '\n')
            {
                At++;
            }

            LineInFile += 1;
            ColInFile   = 0;
        } break;

        case ':':
        {
            At++;

            if (IsValidBuffer(&FileContent) && FileContent.Data[At] == '=')
            {
                CreateThemeToken(ThemeToken_Assignment, LineInFile, ColInFile, &Tokens);
                At++;
            }
            else
            {
                CimLog_Error("Stray ':'. Did you mean := (Assignment)?");
                return Tokens;
            }
        } break;

        case '[':
        {
            At++;

            theme_token *Token = CreateThemeToken(ThemeToken_Vector, LineInFile, ColInFile, &Tokens);

            while (Token->Vector.Size < CimTheme_MaxVectorSize && IsValidBuffer(&FileContent))
            {
                IgnoreWhiteSpaces(&FileContent);

                Char        = FileContent.Data[At++];
                cim_u32 Idx = Token->Vector.Size;
                while (IsValidBuffer(&FileContent) && IsNumberCharacter(Char))
                {
                    Token->Vector.DataU32[Idx] = (Token->Vector.DataU32[Idx] * 10) + (Char - '0');
                    Char                       = FileContent.Data[At++];     
                }

                IgnoreWhiteSpaces(&FileContent);

                if (Char == ',')
                {
                    At++;
                    Token->Vector.Size++;
                }
                else if (Char == ']')
                {
                    break;
                }
                else
                {
                    CimLog_Error("Stray character in vector : %c", Char);
                    return Tokens;
                }
            }

            if (Char != ']')
            {
                CimLog_Error("Vector exceeds maximum size.");
                return Tokens;
            }
        } break;

        case '"':
        {
            At++;

            theme_token *Token = CreateThemeToken(ThemeToken_String, LineInFile, ColInFile, &Tokens);
            Token->Identifier.At = FileContent.Data + At;

            Char = FileContent.Data[At++];
            while (IsValidBuffer(&FileContent) && (IsAlphaCharacter(Char) || IsNumberCharacter(Char)))
            {
                Char = FileContent.Data[At++];
            }

            if (!IsValidBuffer(&FileContent))
            {
                CimLog_Error("End of file reached without closing string.");
                return Tokens;
            }

            if (Char != '"')
            {
                CimLog_Error("Unexpected character found in string.");
                return Tokens;
            }

            At++;

            Token->Identifier.Size = (FileContent.Data + At) - Token->Identifier.At;
        } break;

        default:
        {
            CreateThemeToken((ThemeToken_Type)Char, LineInFile, ColInFile, &Tokens);
            At++;
        } break;

        }

        FileContent.At = At;
    }

    return Tokens;
}

static bool
StoreAttribute(ThemeAttribute_Flag Attribute, theme_token *Value, attribute_parser *Parser)
{
    switch (Parser->State)
    {
    case ThemeParsing_None:
    case ThemeParsing_Count:
    {
        CimLog_Error("Invalid internal parser state.");
        return false;
    }

    case ThemeParsing_Window:
    {
        window_theme *Theme = &Parser->Active.Window;

        CimLog_Info("Setting attribute for window.");

        switch (Attribute)
        {

        case ThemeAttribute_Size:        Theme->Size        = CimTheme_ArrayToVec2(Value->Vector.DataU32); break;
        case ThemeAttribute_Color:       Theme->Color       = CimTheme_ArrayToVec4(Value->Vector.DataU32); break;
        case ThemeAttribute_Padding:     Theme->Padding     = CimTheme_ArrayToVec4(Value->Vector.DataU32); break;
        case ThemeAttribute_Spacing:     Theme->Spacing     = CimTheme_ArrayToVec2(Value->Vector.DataU32); break;
        case ThemeAttribute_BorderColor: Theme->BorderColor = CimTheme_ArrayToVec4(Value->Vector.DataU32); break;
        case ThemeAttribute_BorderWidth: Theme->BorderWidth = Value->UInt32;                               break;

        default:
        {
            CimLog_Error("Invalid attribute supplied to window theme.");
            return false;
        } break;

        }
    } break;

    case ThemeParsing_Button:
    {
        button_theme *Theme = &Parser->Active.Button;

        CimLog_Info("Setting attribute for button.");

        switch (Attribute)
        {

        case ThemeAttribute_Color:       Theme->Color       = CimTheme_ArrayToVec4(Value->Vector.DataU32); break;
        case ThemeAttribute_BorderColor: Theme->BorderColor = CimTheme_ArrayToVec4(Value->Vector.DataU32); break;
        case ThemeAttribute_BorderWidth: Theme->BorderWidth = Value->UInt32;                               break;

        default:
        {
            CimLog_Error("Invalid attribute supplied to button theme.");
            return false;
        } break;

        }
    } break;

    }

    return true;
}

static bool
ParseThemeTokenBuffer(buffer TokenBuffer)
{
    theme_token *Tokens     = (theme_token*)TokenBuffer.Data;
    cim_u32      TokenCount = TokenBuffer.At / sizeof(theme_token); 

    attribute_parser Parser = {};
    Parser.State = ThemeParsing_None;
    Parser.At    = 0;

    while (Parser.At < TokenCount)
    {
        theme_token *Token = Tokens + Parser.At;
        
        switch (Token->Type)
        {

        case ThemeToken_Theme:
        {
            if (Parser.State != ThemeParsing_None)
            {
                CimLog_Error("Forgot { or } somewhere above?");
                return false;
            }

            if (Parser.At + 3 >= TokenCount)
            {
                CimLog_Error("Idk man you fucked up.");
                return false;
            }

            if (Token[1].Type != ThemeToken_String || Token[2].Type != ThemeToken_For || Token[3].Type != ThemeToken_Identifier )
            {
                CimLog_Error("A theme must be set like this: Theme \"NameOfTheme\" for ComponentType");
                return false;
            }

            theme_token *ThemeNameToken     = Token + 1;
            theme_token *ComponentTypeToken = Token + 3;
            
            // NOTE: Could also store an attribute mask?
            typedef struct known_type { const char *Name; size_t Length; ThemeParsing_State State; } known_type;
            known_type KnownTypes[] =
            {
                {"window", sizeof("window") - 1, ThemeParsing_Window},
                {"button", sizeof("button") - 1, ThemeParsing_Button},
            };

            for (cim_u32 KnownIdx = 0; KnownIdx < Cim_ArrayCount(KnownTypes); ++KnownIdx)
            {
                known_type Known = KnownTypes[KnownIdx];

                if (Known.Length != ComponentTypeToken->Identifier.Size)
                {
                    continue;
                }

                cim_u32 CharIdx;
                for (CharIdx = 0; CharIdx < ComponentTypeToken->Identifier.Size; CharIdx++)
                {
                    cim_u8 LowerChar = ToLowerChar(ComponentTypeToken->Identifier.At[CharIdx]);
                    if (LowerChar != Known.Name[CharIdx])
                    {
                        break;
                    }
                }

                if (CharIdx == Known.Length)
                {
                    Parser.State = Known.State;
                    break;
                }
            }

            if (Parser.State == ThemeParsing_None)
            {
                // WARN: A bit cheap.

                if (ThemeNameToken->Identifier.Size > 32)
                {
                    ThemeNameToken->Identifier.Size = 31;
                }

                char ThemeName[32];
                memcpy(ThemeName, ThemeNameToken->Identifier.At, ThemeNameToken->Identifier.Size);

                CimLog_Error("Invalid component type for theme: %s", ThemeName);
                CimLog_Error("Theme must be (Case Insensitive): Window or Button.");
                return false;
            }

            Parser.At += 4;
        } break;

        case ThemeToken_Identifier:
        {
            if (Parser.State == ThemeParsing_None)
            {
                CimLog_Error("Forgot { or } somewhere above?");
                return false;
            }

            if (Parser.At + 2 >= TokenCount)
            {
                CimLog_Error("End of file reached with invalid formatting.");
                return false;
            }

            if (Token[1].Type != ThemeToken_Assignment)
            {
                CimLog_Error("Invalid formatting. Should be -> Attribute := Value.");
                return false;
            }

            theme_token *AttributeToken = Token;
            theme_token *ValueToken     = Token + 2;

            bool HasNegativeValue = false;
            if (Token[2].Type == '-')
            {
                HasNegativeValue = true;

                if (Parser.At + 3 >= TokenCount)
                {
                    CimLog_Error("End of file reached with invalid formatting.");
                    return false;
                }

                if (Token[4].Type != ';')
                {
                    CimLog_Error("Missing ; after attribute.");
                    return false;
                }
            }
            else if (Token[3].Type != ';')
            {
                CimLog_Error("Missing ; after attribute.");
                return false;
            }

            // NOTE: Could use binary search if number of attributes becomes large.
            typedef struct known_type { const char *Name; size_t Length; ThemeAttribute_Flag TypeFlag; cim_bit_field ValidValueTokenMask; } known_type;
            known_type KnownTypes[] =
            {
                {"size"       , sizeof("size")        - 1, ThemeAttribute_Size       , ThemeToken_Number},
                {"color"      , sizeof("color")       - 1, ThemeAttribute_Color      , ThemeToken_Vector},
                {"padding"    , sizeof("padding")     - 1, ThemeAttribute_Padding    , ThemeToken_Vector},
                {"spacing"    , sizeof("spacing")     - 1, ThemeAttribute_Spacing    , ThemeToken_Vector},
                {"borderwidth", sizeof("borderwidth") - 1, ThemeAttribute_BorderWidth, ThemeToken_Number},
                {"bordercolor", sizeof("bordercolor") - 1, ThemeAttribute_BorderColor, ThemeToken_Vector},
            };

            ThemeAttribute_Flag Attribute = ThemeAttribute_None;
            for (cim_u32 KnownIdx = 0; KnownIdx < Cim_ArrayCount(KnownTypes); ++KnownIdx)
            {
                known_type Known = KnownTypes[KnownIdx];

                if (Known.Length != AttributeToken->Identifier.Size)
                {
                    continue;
                }

                cim_u32 CharIdx;
                for (CharIdx = 0; CharIdx < AttributeToken->Identifier.Size; CharIdx++)
                {
                    cim_u8 LowerChar = ToLowerChar(AttributeToken->Identifier.At[CharIdx]);
                    if (LowerChar != Known.Name[CharIdx])
                    {
                        break;
                    }
                }

                if (CharIdx == Known.Length)
                {
                    if (!(ValueToken->Type & Known.ValidValueTokenMask))
                    {
                        CimLog_Error("Invalid formmating for value.");
                        return false;
                    }

                    Attribute = Known.TypeFlag;
                    break;
                }
            }

            if (Attribute == ThemeAttribute_None)
            {
                // WARN: A bit cheap.
                if (AttributeToken->Identifier.Size > 32) AttributeToken->Identifier.Size = 31;
                char AttributeName[32];
                memcpy(AttributeName, AttributeToken->Identifier.At, AttributeToken->Identifier.Size);

                CimLog_Error("Invalid attribute name: %s", AttributeName);
                return false;
            }

            bool Stored = StoreAttribute(Attribute, ValueToken, &Parser);
            if (!Stored)
            {
                return false;
            }

            Parser.At += HasNegativeValue ? 5 : 4;
        } break;

        case '{':
        {
            Parser.At++;

            if (Parser.State == ThemeParsing_None)
            {
                CimLog_Error("You must begin a theme before strating to enumerate its attributes.");
                return false;
            }
        } break;

        case '}':
        {
            Parser.At++;

            if (Parser.State == ThemeParsing_None)
            {
                CimLog_Error("You must begin a theme before strating to enumerate its attributes.");
                return false;
            }

            // TODO: And then one thing we could do is store the theme somewhere
            // when this point is reached. Copy whatever data is in the active?

            Parser.State = ThemeParsing_None;
        } break;

        default:
        {
            CimLog_Error("Found invalid token in file.");
            return false;
        } break;

        }
    }

    return true;
}

static void
InitializeUIThemes(char **Files, cim_u32 FileCount)
{
    if (Files)
    {
        for (cim_u32 FileIdx = 0; FileIdx < FileCount; FileIdx++)
        {
            char *FileName = Files[FileIdx];

            buffer TokenBuffer = TokenizeThemeFile(FileName);
            
            if (TokenBuffer.Data)
            {
                bool ValidContent = ParseThemeTokenBuffer(TokenBuffer);
                if (!ValidContent)
                {
                    CimLog_Error("Invalid content in file: %s. See error above.", FileName);
                }
            }
            else
            {
                CimLog_Error("Failed to tokenize file: %s. See error above.", FileName);
            }
        }
    }
    else
    {
        CimLog_Error("Unable to initialze UI themes because Arg1 is NULL.");
    }
}
