#include "cgl_texture.h"

tagTexture::tagTexture(int width, int height, int bitsPerPixel, const SDL_Color* palette) :
	w(width), h(height), pitch(0),
	genID(0), genFBOID(0), genPBOID(0),
	BitsPerPixel(bitsPerPixel), gfPalette(nullptr)
{
	glGenTextures(1, &genID);
	glGenBuffers(1, &genPBOID);
	int err = glGetError();
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, genID);
	err = glGetError() || err;

	pitch = w * ((bitsPerPixel + 1) >> 3);
	int m_Format= GL_RGBA;
	int m_Type = GL_UNSIGNED_BYTE;
	int m_internalformat = GL_RGBA8;
	switch (bitsPerPixel)
	{
	case  8:
		m_Format = GL_RED;
		m_Type = GL_UNSIGNED_BYTE;
		m_internalformat = GL_R8;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		BitsPerPixel = 8;
		break;
	case 16:
	case 15:
		m_Format = GL_RGBA;
		m_Type = GL_UNSIGNED_SHORT_5_5_5_1;
		m_internalformat = GL_RGB5_A1;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		BitsPerPixel = 16;
		break;
	case 32:
	default:
		m_Format = GL_RGBA;
		m_Type = GL_UNSIGNED_BYTE;
		m_internalformat = GL_RGBA8;

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		BitsPerPixel = 32;
		break;
	}
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	if (BitsPerPixel == 8 && palette)
	{
		if (!gfPalette)
			gfPalette = new PAL_fColor[256];
		for (int n = 0; n < 256; n++)
		{
			ColorTofColor(palette[n], gfPalette[n]);
		}
	};
	glEnable(GL_TEXTURE_2D);
	glTexImage2D(GL_TEXTURE_2D, 0, m_internalformat,
		w, h, 0, m_Format,
		m_Type, NULL);
	err = glGetError() || err;
	glGenerateMipmap(GL_TEXTURE_2D);
	err = glGetError() || err;
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
}

tagTexture::~tagTexture()
{
	glDeleteTextures(1, &genID);
	glDeleteBuffers(1, &genPBOID);
	if (genFBOID)glDeleteFramebuffers(1, &genFBOID);
	if (gfPalette) delete[] gfPalette;
	gfPalette = nullptr;
	genID = genPBOID = genFBOID  = 0;
}

