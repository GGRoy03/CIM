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
static theme_token        *CreateThemeToken        (ThemeToken_Type Type, cim_u32 Line, buffer *TokenBuffer);
static void                IgnoreWhiteSpaces       (buffer *Content);
static theme_parsing_error StoreAttributeInTheme   (ThemeAttribute_Flag Attribute, theme_token *Value, theme_parser *Parser);
static theme_parsing_error ValidateAndStoreThemes  (char *FileName, theme_parser *Parser);
static theme_parsing_error GetNextTokenBuffer      (char *FileName, theme_parser *Parser);
static void                HandleThemeError        (theme_parsing_error *Error, char *FileName, const char *PublicAPIFunctionName);

// [Macros]

#define CimTheme_MaxVectorSize 4
#define CimTheme_ArrayToVec2(Array) {Array[0], Array[1]}
#define CimTheme_ArrayToVec4(Array) {Array[0], Array[1], Array[2], Array[3]}
#define CimTheme_SetErrorMsg(Error, Msg) memcpy(Error.Message, Msg, sizeof(Msg));

// [Public API Implementation]

static void
InitializeUIThemes(char **Files, cim_u32 FileCount)
{
    if (!Files)
    {
        theme_parsing_error Error;
        Error.Type          = ThemeParsingError_Argument;
        Error.ArgumentIndex = 0;
        CimTheme_SetErrorMsg(Error, "Must be a valid pointer of type char**.");

        HandleThemeError(&Error, NULL, "InitializeUIThemes");

        return;
    }

    theme_parser Parser = {};
    Parser.State = ThemeParsing_None;
    Parser.AtLine = 0;

    for (cim_u32 FileIdx = 0; FileIdx < FileCount; FileIdx++)
    {
        char *FileName = Files[FileIdx];

        theme_parsing_error TokenError = GetNextTokenBuffer(FileName, &Parser);
        if (TokenError.Type != ThemeParsingError_None)
        {
            HandleThemeError(&TokenError, FileName, "InitializeUIThemes");
            continue;
        }

        theme_parsing_error ParseError = ValidateAndStoreThemes(FileName, &Parser);
        if (ParseError.Type != ThemeParsingError_None)
        {
            HandleThemeError(&ParseError, FileName, "InitializeUIThemes");
            continue;
        }

        Parser.TokenBuffer.At = 0;

        CimLog_Info("Successfully parsed file: %s", FileName);
    }

    FreeBuffer(&Parser.TokenBuffer);
}

// [Internal Helpers]

static theme_token *
CreateThemeToken(ThemeToken_Type Type, cim_u32 Line, buffer *TokenBuffer)
{
    if (!IsValidBuffer(TokenBuffer))
    {
        size_t NewSize = (TokenBuffer->Size * 2) * sizeof(theme_token);
        buffer Temp    = AllocateBuffer(NewSize);
        Temp.At = TokenBuffer->At;

        if (!Temp.Data)
        {
            // TODO: Report error.
        }

        memcpy(Temp.Data, TokenBuffer->Data, TokenBuffer->Size);

        FreeBuffer(TokenBuffer);

        TokenBuffer->Data = Temp.Data;
        TokenBuffer->Size = NewSize;
        TokenBuffer->At   = Temp.At;
    }

    theme_token *Result = (theme_token*)(TokenBuffer->Data + TokenBuffer->At);
    Result->LineInFile = Line;
    Result->Type       = Type;

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

static theme_parsing_error
GetNextTokenBuffer(char *FileName, theme_parser *Parser)
{
    theme_parsing_error Error = {};
    Error.Type = ThemeParsingError_None;

    buffer FileContent = ReadEntireFile(FileName);
    if(!FileContent.Data)
    {
        Error.Type = ThemeParsingError_Argument;
        CimTheme_SetErrorMsg(Error, "Could not read file given as argument.");

        return Error;
    }

    Parser->TokenBuffer = AllocateBuffer(128 * sizeof(theme_token));
    Parser->AtLine      = 1;


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
            while (IsValidBuffer(&FileContent) && (IsAlphaCharacter(Char) || IsNumberCharacter(Char)))
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
                CreateThemeToken(ThemeToken_Theme, Parser->AtLine, &Parser->TokenBuffer);
            }
            else if (IdLength == 3                &&
                     ToLowerChar(IdPtr[0]) == 'f' &&
                     ToLowerChar(IdPtr[1]) == 'o' &&
                     ToLowerChar(IdPtr[2]) == 'r')
            {
                CreateThemeToken(ThemeToken_For, Parser->AtLine, &Parser->TokenBuffer);
            }
            else
            {
                theme_token *Token = CreateThemeToken(ThemeToken_Identifier, Parser->AtLine, &Parser->TokenBuffer);
                Token->Identifier.At   = IdPtr;
                Token->Identifier.Size = IdLength;
            }
        } break;

        case '0' ... '9':
        {
            theme_token *Token = CreateThemeToken(ThemeToken_Number, Parser->AtLine, &Parser->TokenBuffer);
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

            Parser->AtLine += 1;
        } break;

        case ':':
        {
            At++;

            if (IsValidBuffer(&FileContent) && FileContent.Data[At] == '=')
            {
                CreateThemeToken(ThemeToken_Assignment, Parser->AtLine, &Parser->TokenBuffer);
                At++;
            }
            else
            {
                Error.Type       = ThemeParsingError_Syntax;
                Error.LineInFile = Parser->AtLine;
                CimTheme_SetErrorMsg(Error, "Stray ':'. Did you mean := ?");

                return Error;
            }
        } break;

        case '[':
        {
            At++;

            theme_token *Token = CreateThemeToken(ThemeToken_Vector, Parser->AtLine, &Parser->TokenBuffer);

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
                    Error.Type       = ThemeParsingError_Syntax;
                    Error.LineInFile = Parser->AtLine;
                    snprintf(Error.Message, sizeof(Error.Message), "Found invalid character in vector: %c.", Char);

                    return Error;
                }
            }

            if (Char != ']')
            {
                Error.Type       = ThemeParsingError_Syntax;
                Error.LineInFile = Parser->AtLine;
                CimTheme_SetErrorMsg(Error, "Vector exceeds maximum size (4).");

                return Error;
            }
        } break;

        case '"':
        {
            At++;

            theme_token *Token = CreateThemeToken(ThemeToken_String, Parser->AtLine, &Parser->TokenBuffer);
            Token->Identifier.At = FileContent.Data + At;

            Char = FileContent.Data[At];
            while (IsValidBuffer(&FileContent) && (IsAlphaCharacter(Char) || IsNumberCharacter(Char) || Char == ' '))
            {
                Char = FileContent.Data[++At];
            }

            if (!IsValidBuffer(&FileContent))
            {
                Error.Type       = ThemeParsingError_Syntax;
                Error.LineInFile = Parser->AtLine;
                CimTheme_SetErrorMsg(Error, "End of file reached without closing string.");

                return Error;
            }

            if (Char != '"')
            {
                Error.Type       = ThemeParsingError_Syntax;
                Error.LineInFile = Parser->AtLine;
                CimTheme_SetErrorMsg(Error, "Unexpected character found in string.");

                return Error;
            }

            Token->Identifier.Size = (FileContent.Data + At) - Token->Identifier.At;
            At++;
        } break;

        default:
        {
            CreateThemeToken((ThemeToken_Type)Char, Parser->AtLine, &Parser->TokenBuffer);
            At++;
        } break;

        }

        FileContent.At = At;
    }

    return Error;
}

static theme_parsing_error
StoreAttributeInTheme(ThemeAttribute_Flag Attribute, theme_token *Value, theme_parser *Parser)
{
    theme_parsing_error Error = {};
    Error.Type = ThemeParsingError_None;

    switch (Parser->State)
    {
    case ThemeParsing_None:
    case ThemeParsing_Count:
    {
        theme_parsing_error Error = {};
        Error.Type = ThemeParsingError_Internal;
        CimTheme_SetErrorMsg(Error, "Invalid parser state when trying to set an attribute. Should have been found earlier.");

        return Error;
    }

    case ThemeParsing_Window:
    {
        window_theme *Theme = &Parser->ActiveTheme.Window;

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
            Error.Type       = ThemeParsingError_Syntax;
            Error.LineInFile = Parser->AtLine;           // WARN: Incorrect?
            CimTheme_SetErrorMsg(Error, "Invalid attribute supplied to window theme.");
        } break;

        }
    } break;

    case ThemeParsing_Button:
    {
        button_theme *Theme = &Parser->ActiveTheme.Button;

        CimLog_Info("Setting attribute for button.");

        switch (Attribute)
        {

        case ThemeAttribute_Color:       Theme->Color       = CimTheme_ArrayToVec4(Value->Vector.DataU32); break;
        case ThemeAttribute_BorderColor: Theme->BorderColor = CimTheme_ArrayToVec4(Value->Vector.DataU32); break;
        case ThemeAttribute_BorderWidth: Theme->BorderWidth = Value->UInt32;                               break;

        default:
        {
            Error.Type       = ThemeParsingError_Syntax;
            Error.LineInFile = Parser->AtLine;           // WARN: Incorrect?
            CimTheme_SetErrorMsg(Error, "Invalid attribute supplied to button theme.")
        } break;

        }
    } break;

    }

    return Error;
}

static theme_parsing_error
ValidateAndStoreThemes(char *FileName, theme_parser *Parser)
{
    theme_parsing_error Error = {};
    Error.Type = ThemeParsingError_None;

    theme_token *Tokens     = (theme_token *)Parser->TokenBuffer.Data;;
    cim_u32      TokenCount = Parser->TokenBuffer.At / sizeof(theme_token);

    while (Parser->AtToken< TokenCount)
    {
        theme_token *Token = Tokens + Parser->AtToken;

        switch (Token->Type)
        {

        case ThemeToken_Theme:
        {
            if (Parser->State != ThemeParsing_None)
            {
                Error.Type       = ThemeParsingError_Syntax;
                Error.LineInFile = Parser->AtLine;
                CimTheme_SetErrorMsg(Error, "Forgot to close previous theme with } ?");

                return Error;
            }

            if (Parser->AtToken + 3 >= TokenCount)
            {
                Error.Type       = ThemeParsingError_Syntax;
                Error.LineInFile = Parser->AtLine;
                CimTheme_SetErrorMsg(Error, "Unexpected end of file.");

                return Error;
            }

            if (Token[1].Type != ThemeToken_String || Token[2].Type != ThemeToken_For || Token[3].Type != ThemeToken_Identifier)
            {
                Error.Type       = ThemeParsingError_Syntax;
                Error.LineInFile = Token->LineInFile;
                CimTheme_SetErrorMsg(Error, "A theme must be set like this: Theme \"NameOfTheme\" for ComponentType");

                return Error;
            }

            theme_token *ThemeNameToken = Token + 1;
            theme_token *ComponentTypeToken = Token + 3;

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
                    Parser->State = Known.State;
                    break;
                }
            }

            if (Parser->State == ThemeParsing_None)
            {
                Error.Type       = ThemeParsingError_Syntax;
                Error.LineInFile = ComponentTypeToken->LineInFile;

                // BUG: Can easily overflow.
                char ThemeName[64] = { 0 };
                memcpy(ThemeName, ThemeNameToken->Identifier.At, ThemeNameToken->Identifier.Size);

                snprintf(Error.Message, sizeof(Error.Message), "Invalid component type for theme: %s", ThemeName);

                return Error;
            }

            Parser->AtToken+= 4;
        } break;

        case ThemeToken_Identifier:
        {
            if (Parser->State == ThemeParsing_None)
            {
                Error.Type       = ThemeParsingError_Syntax;
                Error.LineInFile = Token->LineInFile;

                // BUG: Can easily overflow.
                char IdentifierString[64];
                memcpy(IdentifierString, Token->Identifier.At, Token->Identifier.Size);

                snprintf(Error.Message, sizeof(Error.Message), "Found identifier: %s | Outside of a theme block.",
                         IdentifierString);

                return Error;
            }

            if (Parser->AtToken+ 2 >= TokenCount)
            {
                Error.Type       = ThemeParsingError_Syntax;
                Error.LineInFile = Token->LineInFile;

                CimTheme_SetErrorMsg(Error, "End of file reached earlier than expected.");
                return Error;
            }

            if (Token[1].Type != ThemeToken_Assignment)
            {
                Error.Type       = ThemeParsingError_Syntax;
                Error.LineInFile = Token[1].LineInFile;

                CimTheme_SetErrorMsg(Error, "Invalid formatting. Should be -> Attribute := Value.");
                return Error;
            }

            theme_token *AttributeToken = Token;
            theme_token *ValueToken     = Token + 2;

            bool HasNegativeValue = false;
            if (Token[2].Type == '-')
            {
                HasNegativeValue = true;

                if (Parser->AtToken+ 3 >= TokenCount)
                {
                    Error.Type       = ThemeParsingError_Syntax;
                    Error.LineInFile = Token[2].LineInFile;

                    CimTheme_SetErrorMsg(Error, "End of file reached with invalid formatting.");
                    return Error;
                }

                if (Token[4].Type != ';')
                {
                    Error.Type       = ThemeParsingError_Syntax;
                    Error.LineInFile = Token[4].LineInFile;

                    CimTheme_SetErrorMsg(Error, "Missing ; after setting the attribute value.");
                    return Error;
                }
            }
            else if (Token[3].Type != ';')
            {
                Error.Type       = ThemeParsingError_Syntax;
                Error.LineInFile = Token[3].LineInFile;
                CimTheme_SetErrorMsg(Error, "Missing ; after setting the attribute value.");

                return Error;
            }

            /* NOTE: Could use binary search if number of attributes becomes large. */
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
                        Error.Type       = ThemeParsingError_Syntax;
                        Error.LineInFile = ValueToken->LineInFile;

                        snprintf(Error.Message, sizeof(Error.Message), "Attribute of type: %s does not accept this format for values.", Known.Name);

                        return Error;
                    }

                    Attribute = Known.TypeFlag;
                    break;
                }
            }

            if (Attribute == ThemeAttribute_None)
            {
                Error.Type = ThemeParsingError_Syntax;
                Error.LineInFile = Parser->AtLine;

                // BUG: Can easily overflow.
                char AttributeName[64] = { 0 };
                memcpy(AttributeName, AttributeToken->Identifier.At, AttributeToken->Identifier.Size);

                snprintf(Error.Message, sizeof(Error.Message), "Invalid attribute name: %s", AttributeName);

                return Error;
            }

            theme_parsing_error Error = StoreAttributeInTheme(Attribute, ValueToken, Parser);
            if (Error.Type != ThemeParsingError_None)
            {
                return Error;
            }

            Parser->AtToken += HasNegativeValue ? 5 : 4;
        } break;

        case '{':
        {
            Parser->AtToken++;

            if (Parser->State == ThemeParsing_None)
            {
                Error.Type       = ThemeParsingError_Syntax;
                Error.LineInFile = Token->LineInFile;
                CimTheme_SetErrorMsg(Error, "You must end a theme block '}' before beginning a new one '{'.");

                return Error;
            }
        } break;

        case '}':
        {
            Parser->AtToken++;

            if (Parser->State == ThemeParsing_None)
            {
                Error.Type       = ThemeParsingError_Syntax;
                Error.LineInFile = Token->LineInFile;
                CimTheme_SetErrorMsg(Error, "You must begin a theme block '{' before closing it '}'.");

                return Error;
            }

            /* TODO: And then one thing we could do is store the theme somewhere
               when this point is reached. Copy whatever data is in the active? */

            Parser->State = ThemeParsing_None;
        } break;

        default:
        {
            // WARN: The error message could be wrong? Depends if we correctly handle
            // earlier cases I guess?

            Error.Type       = ThemeParsingError_Syntax;
            Error.LineInFile = Token->LineInFile;
            snprintf(Error.Message, sizeof(Error.Message), "Found invalid token in file: %c", (char)Token->Type);

            return Error;
        } break;

        }
    }

    return Error;
}

static void
HandleThemeError(theme_parsing_error *Error, char *FileName, const char *PublicAPIFunctionName)
{
    switch (Error->Type)
    {

    case ThemeParsingError_Syntax:
    {
        CimLog_Error("Syntax Error: In file %s, at line %u -> %s",
                     FileName, Error->LineInFile, Error->Message);
    } break;

    case ThemeParsingError_Argument:
    {
        CimLog_Error("Argument Error: When calling %s argument %u is invalid -> %s",
                      PublicAPIFunctionName, Error->ArgumentIndex, Error->Message);
    } break;

    case ThemeParsingError_Internal:
    {
        CimLog_Error("Internal Error. Please report it at: https://github.com/GGRoy03/CIM/. Error: %s",
                     Error->Message);
    } break;

    }
}