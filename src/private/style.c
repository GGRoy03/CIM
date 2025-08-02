#include "private/cim_private.h"

#include <stdio.h>

typedef enum CimStyleToken_Type
{
    CimStyleToken_Identifier = 256,
    CimStyleToken_String     = 257,
    CimStyleToken_Number     = 258,
    CimStyleToken_Assignment = 259,
    CimStyleToken_Unknown    = 260,
} CimStyleToken_Type;

// NOTE: Buffers are general purpose.
typedef struct cim_buffer
{
    cim_u8* Data;
    cim_u32 At;
    cim_u32 Size;
} cim_buffer;

typedef struct cim_style_token
{
    CimStyleToken_Type Type;

    cim_u32 LineStart;
    cim_u32 CharStart;

    union
    {
        char   *Name;
        cim_i32 SignedInt32Value;
        cim_f32 Float32Value;
    };

    cim_u32 IdentifierLength;
} cim_style_token;

typedef struct cim_style_lexer
{
    cim_style_token *Tokens;
    cim_u32          TokenCount;
    cim_u32          TokenCapacity;

    cim_u32 LineNumber;
    cim_u32 CharNumber;
} cim_style_lexer;

// [Internal Functions]

static cim_buffer
ReadEntireFile(const char *File)
{
    cim_buffer Result = { 0 };

    FILE *FilePointer = fopen(File, "rb");
    if (!FilePointer) 
    {
        CimLog_Error("Failed to open the styling file | for: %s", File);
        return;
    }
    if (fseek(FilePointer, 0, SEEK_END) != 0)
    {
        CimLog_Error("Failed to seek the EOF | for: %s", File);
        fclose(FilePointer);
        return;
    }

    Result.Size = (cim_u32)ftell(FilePointer);
    if (Result.Size < 0)
    {
        CimLog_Error("Failed to tell the size of the file | for: %s", File);
        return;
    }

    fseek(FilePointer, 0, SEEK_SET);

    Result.Data = malloc((size_t)Result.Size);
    if (!Result.Data)
    {
        CimLog_Error("Failed to heap-allocate | for: %s, with size: %d", File, Result.Size);
        fclose(FilePointer);
        return;
    }

    size_t Read = fread(Result.Data, 1, (size_t)Result.Size, FilePointer);
    if (Read != (size_t)Result.Size)
    {
        CimLog_Error("Could not read the full file | for: %s", File);
        free(Result.Data);
        fclose(FilePointer);
        return;
    }

    fclose(FilePointer);

    return Result;
}

static inline bool
CanReadFromBuffer(cim_buffer *Buffer, cim_u32 At)
{
    bool Result = At < Buffer->Size;
    return Result;
}

static inline cim_style_token *
AllocateToken(cim_style_lexer *Lexer)
{
    if (Lexer->TokenCount < Lexer->TokenCapacity)
    {
        cim_style_token *Token = Lexer->Tokens + Lexer->TokenCount++;
        return Token;
    }
    else
    {
        CimLog_Error("Token count overflow while parsing.");
        return NULL;
    }
}

static const char *
TokenTypeAsString(CimStyleToken_Type Type)
{
    switch (Type)
    {

    case CimStyleToken_Identifier:
    {
        return "Identifier";
    } break;

    case CimStyleToken_String:
    {
        return "String";
    } break;

    case CimStyleToken_Number:
    {
        return "Number";
    } break;

    case CimStyleToken_Assignment:
    {
        return "Assignment";
    } break;

    case CimStyleToken_Unknown:
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

// [API Functions]

#ifdef __cplusplus
extern "C" {
#endif

void
CimStyle_ParseFile(const char *File)
{
    cim_buffer      Content = ReadEntireFile(File);
    cim_style_lexer Lexer = { 0 };

    if (Content.Data)
    {
        Lexer.LineNumber    = 1;
        Lexer.Tokens        = malloc(1000 * sizeof(cim_style_token));
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
                while (CanReadFromBuffer(&Content, At) && IsIdentifier(Content.Data[At]))
                {
                    At++;
                }

                cim_style_token *Token = AllocateToken(&Lexer);
                Token->Type      = CimStyleToken_Identifier;
                Token->LineStart = Lexer.LineNumber;
                Token->CharStart = Lexer.CharNumber;
            }
            else if (*Character == ':' && Character[1] == '=')
            {
                At++;

                cim_style_token *Token = AllocateToken(&Lexer);
                Token->Type      = CimStyleToken_Assignment;
                Token->LineStart = Lexer.LineNumber;
                Token->CharStart = Lexer.CharNumber;
            }
            else if (*Character == '"')
            {
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


            } break;

            }

            Content.At        = At;
            Lexer.CharNumber += 1;
        }
    }

    for (cim_u32 TokIdx = 0; TokIdx < Lexer.TokenCount; TokIdx++)
    {
        cim_style_token *Token = Lexer.Tokens + TokIdx;

        const char *TokenName = TokenTypeAsString(Token->Type);
        CimLog_Info("Parsed token: Type=%s, Line=%u, Character=%u",
                    TokenName, Token->LineStart, Token->CharStart);
    }

    CimLog_Info("Finished parsing file: %s", File);
}

#ifdef __cplusplus
}
#endif
