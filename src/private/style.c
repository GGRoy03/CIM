#include "private/cim_private.h"

#include <stdio.h>

typedef enum Token_Type
{
    Token_Identifier      = 256,
    Token_String          = 257,
    Token_Number          = 258,
    Token_AssignmentOp    = 259,
    Token_Component       = 260,
    Token_ComponentTypeOp = 261,
    Token_Unknown         = 262,
} StyleToken_Type;

// NOTE: Buffers are general purpose.
typedef struct buffer
{
    cim_u8* Data;
    cim_u32 At;
    cim_u32 Size;
} buffer;

typedef struct style_token
{
    StyleToken_Type Type;

    cim_u32 LineStart;
    cim_u32 CharStart;

    union
    {
        cim_i32 SignedInt32Value;
        cim_f32 Float32Value;
        struct { cim_u8 *StringStart; cim_u32 StringLength; };
    };
} style_token;

typedef struct lexer
{
    // The token stream
    style_token *Tokens;
    cim_u32      TokenCount;
    cim_u32      TokenCapacity;

    // Where we are : when tokenizing
    cim_u32 LineNumber;
    cim_u32 CharNumber;

    // Where we are : when parsing
    cim_u32 AtToken;

} lexer;

// [Internal Functions]

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

static inline bool
CanReadFromBuffer(buffer *Buffer, cim_u32 At)
{
    bool Result = At < Buffer->Size;
    return Result;
}

static inline style_token *
AllocateToken(StyleToken_Type Type, cim_u32 L0, cim_u32 C0, lexer *Lexer)
{
    if (Lexer->TokenCount < Lexer->TokenCapacity)
    {
        style_token *Token = Lexer->Tokens + Lexer->TokenCount++;
        Token->Type      = Type;
        Token->LineStart = L0;
        Token->CharStart = C0;

        return Token;
    }
    else
    {
        CimLog_Error("Token count overflow while parsing.");
        return NULL;
    }
}

static const char *
TokenTypeAsString(StyleToken_Type Type)
{
    switch (Type)
    {

    case Token_Identifier:
    {
        return "Identifier";
    } break;

    case Token_String:
    {
        return "String";
    } break;

    case Token_Number:
    {
        return "Number";
    } break;

    case Token_AssignmentOp:
    {
        return "Assignment Op";
    } break;

    case Token_Component:
    {
        return "Component";
    } break;

    case Token_ComponentTypeOp:
    {
        return "Component Type Op";
    } break;

    case Token_Unknown:
    {
        return "Unknown";
    } break;

    default:
        return "No clue";
    }
}

static inline bool
IsIdentifier(cim_u8 Character)
{
    bool Result = (Character >= 'A' && Character <= 'Z') ||
                  (Character >= 'a' && Character <= 'z');

    return Result;
}

static inline style_token *
PeekToken(cim_u32 LookAhead, lexer *Lexer)
{
    if(Lexer->AtToken + LookAhead > Lexer->TokenCapacity)
    {
        CimLog_Error("Tried to read past the end of the token buffer while parsing.");
        return NULL;
    }

    style_token *Token = Lexer->Tokens + Lexer->AtToken + LookAhead;

    return Token;
}

static inline void
EatToken(cim_u32 EatCount, lexer *Lexer)
{
    if(Lexer->AtToken + EatCount > Lexer->TokenCapacity)
    {
        CimLog_Error("Tried to eat tokens past the end of the token buffer.");
        return;
    }

    Lexer->AtToken += EatCount;
}

static inline bool
TokenStreamEnded(lexer *Lexer)
{
    bool Ended = Lexer->AtToken == Lexer->TokenCapacity;
}

static inline void
ParserPrototype(lexer *Lexer)
{
    while (!TokenStreamEnded(Lexer))
    {
        // TODO: Do something with the tokens (Validate and whatnot)
    }
}

// [API Functions]

#ifdef __cplusplus
extern "C" {
#endif

void
CimStyle_ParseFile(const char *File)
{
    buffer Content = ReadEntireFile(File);
    lexer  Lexer   = { 0 };

    if (Content.Data)
    {
        Lexer.LineNumber    = 1;
        Lexer.Tokens        = malloc(1000 * sizeof(style_token));
        Lexer.TokenCapacity = 1000;

        while (CanReadFromBuffer(&Content, Content.At))
        {
            while (Content.Data[Content.At] == ' ')
            {
                Lexer.CharNumber += 1;
                Content.At       += 1;
            }

            cim_u8 *Character = Content.Data + Content.At;
            cim_u32 StartAt   = Content.At;
            cim_u32 At        = StartAt;

            if (IsIdentifier(*Character))
            {
                cim_u32 StringStart = At;

                At++;
                while (CanReadFromBuffer(&Content, At) &&
                       IsIdentifier(Content.Data[At]))
                {
                    At++;
                }

                style_token *Token = AllocateToken(Token_Identifier, Lexer.LineNumber,
                                                   Lexer.CharNumber, &Lexer);
                Token->StringStart  = Content.Data + StringStart;
                Token->StringLength = At - StringStart;
            }
            else if (*Character == ':' && Character[1] == '=')
            {
                At++;

                AllocateToken(Token_AssignmentOp, Lexer.LineNumber, Lexer.CharNumber,
                              &Lexer);
            }
            else if (*Character == '"')
            {
                At++;

                cim_u32 StringStart = At;
                while(CanReadFromBuffer(&Content, At) && Content.Data[At] != '"')
                {
                    At++;
                }

                style_token *Token = AllocateToken(Token_String, Lexer.LineNumber,
                                                   Lexer.CharNumber, &Lexer);
                Token->StringStart  = Content.Data + StringStart;
                Token->StringLength = At - StringStart;
            }
            else if(*Character == '-' && Character[1] == '>')
            {
                At += 2;

                AllocateToken(Token_ComponentTypeOp, Lexer.LineNumber,
                              Lexer.CharNumber, &Lexer);
            }

            Content.At = At;
            Character  = Content.Data + Content.At;

            switch (*Character)
            {

            case '\r':
            case '\n':
            {
                At++;

                if (*Character == '\r' && Content.Data[At] == '\n')
                {
                    At++;
                }

                Lexer.LineNumber += 1;
                Lexer.CharNumber = 0;
            } break;

            case '#':
            {
                At++;

                while (Content.Data[At] != '\n' && Content.Data[At] != '\r')
                {
                    At++;
                }
            } break;

            default:
            {
                At++;

                AllocateToken(*Character, Lexer.LineNumber, Lexer.CharNumber, &Lexer);
            } break;

            }

            Content.At        = At;
            Lexer.CharNumber += 1;
        }
    }

    for (cim_u32 TokIdx = 0; TokIdx < Lexer.TokenCount; TokIdx++)
    {
        style_token *Token = Lexer.Tokens + TokIdx;

        const char *TokenName = TokenTypeAsString(Token->Type);
        CimLog_Info("Parsed token: Type=%s, Line=%u, Character=%u",
                    TokenName, Token->LineStart, Token->CharStart);
    }

    CimLog_Info("Finished parsing file: %s", File);
}

#ifdef __cplusplus
}
#endif
