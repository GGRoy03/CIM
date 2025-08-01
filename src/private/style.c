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

#ifdef __cplusplus
extern "C" {
#endif

void
CimStyle_ParseFile(const char *File)
{
}

#ifdef __cplusplus
}
#endif
