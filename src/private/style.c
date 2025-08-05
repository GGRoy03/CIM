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
    Attribute_HeaderColor,
    Attribute_BackgroundColor,
} Attribute_Type;

typedef enum Token_Type
{
    Token_String     = 255,
    Token_Identifier = 256,
    Token_Number     = 257,
    Token_Assignment = 258,
    Token_Vector     = 269,
} Token_Type;

typedef enum ValueFormat_Flag
{
    ValueFormat_SNumber = 1 << 0,
    ValueFormat_UNumber = 1 << 1,
    ValueFormat_Vector  = 1 << 2,
} ValueFormat_Flag;

// [Structs]

typedef struct style_entry
{
    Attribute_Type Type;
    union
    {
        cim_i32     Int32;
        cim_u32     UInt32;
        cim_f32     Float32;
        cim_vector4 Vector;
    };
} style_entry;

typedef struct style_desc
{
    char           Id[64];
    Component_Flag ComponentFlag;
    style_entry   *Entries;
    size_t         EntryCount;
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
    // A buffer of entries which the Desc.Entries point to the first one in the list.
    style_entry *Entries;
    cim_u32      EntryCount;
    cim_u32      EntrySize;

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
    const char *Name;
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
    {"BackgroundColor", (sizeof("BackgroundColor") - 1), Attribute_BackgroundColor,
    ValueFormat_Vector, Component_Window},

    {"HeaderColor", (sizeof("HeaderColor") - 1), Attribute_HeaderColor,
    ValueFormat_Vector, Component_Window},
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
            CimLog_Fatal("Failed to heap-allocate a buffer for the tokens. OOM?");
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
// 1) Parse hex numbers to vectors.

static lexer
CreateTokenStreamFromBuffer(buffer *Content)
{
    lexer Lexer = { 0 };
    Lexer.Tokens        = malloc(1000 * sizeof(token));
    Lexer.TokenCapacity = 1000;
    Lexer.ValidStream   = false;

    if (!Content->Data || !Content->Size)
    {
        CimLog_Fatal("Cannot parse the file because the file is empty or wasn't read.");
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
            Token->NameLength = Identifier - Character;

            At += Token->NameLength;
        }
        else if (IsNumberCharacter(*Character))
        {
            // TODO: Parse the number.
            token *Token = CreateToken(Token_Number, &Lexer);
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
                CimLog_Fatal("...");
                return;
            }

            Token->NameLength = (Content->Data + At++) - Token->Name;
        }
        else if (*Character == '#')
        {
            At++;

            cim_u32 MaximumHex = 8; // (#RRGGBBAA)
            cim_u32 HexCount   = 0;
            cim_u8 *HexPtr     = Content->Data + At;
            cim_u32 UValue     = 0;

            bool IsDigit   = IsNumberCharacter(*HexPtr);
            bool IsHexChar = IsAlphaCharacter(*HexPtr);

            if (!IsDigit && !IsHexChar)
            {
                CimLog_Fatal("...");
                return;
            }

            while (IsDigit || IsHexChar)
            {
                if (IsDigit)
                {
                    UValue = (UValue * (HexCount * 10)) + (*HexPtr - '0');
                }
                else if (IsHexChar)
                {
                    cim_u32 HexCharValue = *HexPtr - 'A';
                    if (HexCharValue > ('F' - 'A'))
                    {
                        HexCharValue = (*HexPtr - 'a');
                    }

                    HexCharValue += 10;

                    UValue = (UValue * (HexCount * 10)) + HexCharValue;
                }

                ++HexCount;
                ++HexPtr;

                if (HexCount > MaximumHex)
                {
                    break;
                }

                IsDigit   = IsNumberCharacter(*HexPtr);
                IsHexChar = IsAlphaCharacter(*HexPtr);
            }

            At += HexCount;

            if (IsNumberCharacter(Content->Data + At) || IsAlphaCharacter(Content->Data[Content->At]))
            {
                CimLog_Fatal("...");
                return;
            }

            token *Token = CreateToken(Token_Vector, &Lexer);
            Token->Vector = (cim_vector4){ ((UValue << 24) & 0xFF), ((UValue << 16) & 0xFF), ((UValue << 8) & 0xFF), ((UValue << 0) & 0xFF) };
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
// 2) Returns an array of style desc that we can then parse to set the styles.

static user_styles
CreateUserStyles(lexer *Lexer)
{
    user_styles Styles = { 0 };
    Styles.Descs     = malloc(10 * sizeof(style_desc));
    Styles.DescSize  = 100;
    Styles.Entries   = malloc(50 * sizeof(style_entry));
    Styles.EntrySize = 50;
    Styles.IsValid   = false;

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
                CimLog_Error("Strings are only valid in this case: (String Ass Comp)");
                return;
            }

            Next    = Lexer->Tokens + AtToken;
            AtToken += 1;

            if (Next->Type != Token_Identifier)
            {
                CimLog_Error("Strings are only valid in this case: (String Ass Comp)");
                return;
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

                    memcpy(Desc, Next->Name, Next->NameLength);
                    Desc->ComponentFlag = Valid->ComponentFlag;

                    IsValidComponentName = true;

                    break;
                }
            }

            if (!IsValidComponentName)
            {
                CimLog_Fatal("...");
                return;
            }

            Next     = Lexer->Tokens + AtToken;
            AtToken += 1;

            if (Next->Type != ';')
            {
                CimLog_Fatal("...");
                return;
            }
        } break;

        case Token_Identifier:
        {
            valid_attribute Attr  = { 0 };
            bool            Found = false;

            style_desc *Desc = Styles.Descs + (Styles.DescCount - 1);
            if (Styles.DescCount == 0 || Desc->ComponentFlag == Component_Invalid && !Desc)
            {
                CimLog_Fatal("...");
                return;
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
                CimLog_Fatal("...");
                return;
            }

            token *Next = Lexer->Tokens + AtToken;
            AtToken += 1;

            if (Next->Type != Token_Assignment)
            {
                CimLog_Fatal("...");
                return;
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

            // BUG: Can overflow.
            style_entry *Entry = Styles.Entries + Styles.EntryCount;
            Entry->Type = Attr.Type;

            if(Next->Type == Token_Number && (Attr.FormatFlags & ValueFormat_UNumber))
            {
                if(IsNegative)
                {
                    CimLog_Fatal("...");
                    return;
                }

                Entry->UInt32 = Next->UInt32;
            }
            else if (Next->Type == ValueFormat_SNumber && (Attr.FormatFlags & ValueFormat_SNumber))
            {
                Entry->Int32 = Next->Int32;
            }
            else if(Next->Type == Token_Vector && (Attr.FormatFlags & ValueFormat_Vector))
            {
                if(IsNegative)
                {
                    CimLog_Fatal("...");
                    return;
                }

                Entry->Vector = Next->Vector;
            }
            else
            {
                CimLog_Fatal("...");
                return;
            }

        } break;

        }

    }

    Styles.IsValid = true;
    return Styles;
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
        CimLog_Fatal("Cannot initialize styles. Reason: See Error(s) Above");
        return;
    }

    lexer Lexer = CreateTokenStreamFromBuffer(&FileContent);
    if (!Lexer.IsValid)
    {
        CimLog_Fatal("Cannot initialize styles. Reason: See Error(s) Above");
        return;
    }

    user_styles Styles = CreateUserStyles(&Lexer);
    if(!Styles.IsValid)
    {
        CimLog_Fatal("Cannot initialize styles. Reason: See Error(s) Above");
        return;
    }
}

#ifdef __cplusplus
}
#endif
