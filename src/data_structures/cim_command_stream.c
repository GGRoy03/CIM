// ===================================================================================
//
// [DESCRIPTION]
// -------------
// A command stream is really similar to a dyanmic buffer, except that it is a much
// more limited kind and more specific. The command stream is used by the backend
// to render the full UI. This is a WIP file.
//
// [STRUCTURES]
// ------------
// The representation of a draw command the renderer can understand
// struct cim_draw_command
//  VtxOffset    -> Draw call parameter.
//  IdxOffset    -> Draw call parameter.
//  IdxCount     -> Draw call parameter.
//  VtxCount     -> A convenience used by the geometry builder to write the indices.
//  ClippingRect -> To set the clipping rect when drawing.
//  Features     -> To query the correct pipeline in the backend.
//
// The main stream structure
// struct cim_command_stream
//  Source      -> A buffer to holds commands.
//  Capacity    -> Capacity of the source.
//  WriteOffset -> Track where we are writing in the source.
//  ReadOffset  -> Track where we are reading in the source.
//
// [FUNCTIONS]
// -----------
// Command * CimCommandStream_Read(Count, Stream) 
//   Read {Count} commands from the stream. 
//   Returns the command at Stream.WriteOffset.
//
// void CimCommandStream_Write(Count, Commands[], Stream) 
//   Write {Count} commands to {Stream} From {Commands}
//
// void CimCommandStream_Reset(Stream)
//   Resets the offset from the {Stream}
//
// ===================================================================================

#define CimDefault_CmdStreamSize 32 * sizeof(cim_draw_command)
#define CimDefault_CmdStreamGrowShift 1

typedef struct cim_draw_command
{
    cim_u32 VtxOffset;
    cim_u32 IdxOffset;
    cim_u32 IdxCount;
    cim_u32 VtxCount;

    cim_rect      ClippingRect;
    cim_bit_field Features;
} cim_draw_command;

typedef struct cim_command_stream
{
    cim_draw_command *Source;
    cim_u32           Capacity;

    cim_u32 WriteOffset;
    cim_u32 ReadOffset;
} cim_command_stream;

inline cim_draw_command *
CimCommandStream_Read(cim_u32             ReadCount,
                      cim_command_stream *Stream)
{
    if(Stream->ReadOffset + ReadCount > Stream->Capacity)
    {
        Cim_Assert(!"Attempted to read past the end of the command stream.\n");
        return NULL;
    }

    cim_u32 CurrentOffset = Stream->ReadOffset;
    Stream->ReadOffset   += ReadCount;

    cim_draw_command *Command = Stream->Source + CurrentOffset;
    return Command;
}

inline void
CimCommandStream_Write(cim_u32             WriteCount,
                       cim_draw_command   *Commands,
                       cim_command_stream *Stream)
{
    if(Stream->WriteOffset + WriteCount > Stream->Capacity)
    {
        Stream->Capacity = Stream->Capacity ? 
            Stream->Capacity + (Stream->Capacity >> CimDefault_CmdStreamGrowShift) :
            CimDefault_CmdStreamSize
        ;

        while(Stream->Capacity < Stream->WriteOffset + WriteCount)
        {
            Stream->Capacity += (Stream->Capacity >> CimDefault_CmdStreamGrowShift);
        }

        void *New = malloc(Stream->Capacity);
        if(!New)
        {
            Cim_Assert(!"Malloc failure: OOM?");
            return NULL;
        }

        if (Stream->Memory)
        {
            memcpy(New, Stream->Memory, Stream->WriteOffset * sizeof(cim_draw_command));
            free(Stream->Memory);
        }

        Stream->Memory = New;
    }

    for(cim_u32 CmdIdx = 0; CmdIdx < WriteCount; CmdIdx)
    {
        Stream->Source[Stream->WriteOffset++] = Commands[CmdIdx];
    }
}

inline void
CimCommandStream_Reset(cim_command_stream *Stream)
{
    Stream->ReadOffset  = 0;
    Stream->WriteOffset = 0;
}
