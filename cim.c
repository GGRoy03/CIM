#include "cim.h"
#include "cim_private.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#include "third_party/stb_image.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ============================================================
// PUBLIC API TYPE DEFINITIONS FOR CIM. BY SECTION.
// -[SECTION]: Material
// ============================================================
// ============================================================

// -[SECTION:Material] {

// WARN: This function is incomplete.
cim_texture Cim_LoadTextureFromDisk(const char *FileName)
{
	cim_texture Texture = { 0 };
	Texture.Pixels = stbi_load(FileName, &Texture.Width, &Texture.Height, &Texture.Channels, 4);

	if (!Texture.Pixels)
	{
		// NOTE: Probably a better way to handle this error?
		abort();
		return Texture;
	}

	// NOTE: Is it always 4 byte per pixel?
	Texture.Pitch    = Texture.Width * 4;
	Texture.DataSize = (size_t)Texture.Width * Texture.Height * 4;

	return Texture;
}

void Cim_CreateMaterial(const char *ID, cim_bit_field Features)
{
	cim_render_command_header Header;
	Header.Type = CimRenderCommand_CreateMaterial;
	Header.Size = (cim_u32)sizeof(cim_payload_create_material);

	cim_payload_create_material Payload;
	Payload.Features = Features;
	strcpy(Payload.UserID, ID);

	CimInt_PushRenderCommand(&Header, &Payload);
}

void Cim_BindMaterial(const char *ID)
{
	cim_render_command_header Header;
	Header.Type = CimRenderCommand_BindMaterial;
	Header.Size = (cim_u32)sizeof(cim_payload_bind_material);

	cim_payload_bind_material Payload;
	strcpy(Payload.UserID, ID);

	CimInt_PushRenderCommand(&Header, &Payload);
}

void Cim_DestroyMaterial(const char *ID)
{
	cim_render_command_header Header;
	Header.Type = CimRenderCommand_DestroyMaterial;
	Header.Size = (cim_u32)sizeof(cim_payload_destroy_material);

	cim_payload_destroy_material Payload;
	strcpy(Payload.UserID, ID);

	CimInt_PushRenderCommand(&Header, &Payload);
}

// } -[SECTION:Materia]

bool Window(const char *Id)
{
    cim_u32 Hashed = Cim_HashString(Id);

    return true;
}

bool Button(const char *Id)
{
    return true;
}

void Text(const char *Id)
{
	return;
}


#ifdef __cplusplus
}
#endif