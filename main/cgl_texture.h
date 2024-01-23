#ifndef CGLTEXTURE_H
#define CGLTEXTURE_H

#ifdef _HAS_STD_BYTE
#undef _HAS_STD_BYTE
#endif
#define _HAS_STD_BYTE 0

#define  GLAD
//#define GLEW

#ifdef  GLEW  
#define GLEW_STATIC
#endif

#include "command.h"
//#include <Windows.h>

#ifdef GLAD
#include <glad/glad.h>
#else

#ifdef GLEW
#include <GL/glew.h>

#else 

#include <GL/GL.h>
#include <GL/GLU.h>
#define GL_GLEXT_PROTOTYPES
#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>
#endif

#endif

#include <SDL.h>
#include <vector>

using namespace std;

//系统基本显示画面尺度
#define PictureWidth   320
#define PictureHeight  200

#define RealWidth  640
#define RealHeight 400

#define WindowWidth  800
#define WindowHeight 500


#define Div255 0.003921568628
//保持图片显示比例 
//const int KeepAspectRatio = 1;

#define  PictureRatio  ((float)RealWidth/(float)PictureWidth )
typedef struct tagTexture CGL_Texture;

typedef struct PAL_fColor
{
	GLfloat r;
	GLfloat g;
	GLfloat b;
	GLfloat a;
} PAL_fColor;

typedef struct tagTexture
{
	int w;                      /**< The width of the texture */
	int h;                      /**< The height of the texture */
	int pitch;					/**<                           */
	GLuint	genID;//纹理ID
	GLuint	genFBOID;//屏幕缓存区ID
	GLuint  genPBOID;//纹理缓存区ID
	GLuint  BitsPerPixel;//每个像素占用Bit数，目前支持8和32位
	PAL_fColor* gfPalette{nullptr};//调色版

public:
	tagTexture(int width, int height, int bitsPerPixel, const SDL_Color* palette = NULL);

	~tagTexture();;

	inline VOID ColorTofColor(const SDL_Color& c, PAL_fColor& a)
	{
		a.r = c.r * Div255;
		a.g = c.g * Div255;
		a.b = c.b * Div255;
		a.a = c.a * Div255;
	}
} CGL_Texture;

inline VOID setPictureRatio(SDL_Rect *rect)
{
	rect->x *= PictureRatio;
	rect->y *= PictureRatio;
	rect->w *= PictureRatio;
	rect->h *= PictureRatio;
}

#endif // CGLTEXTURE_H
