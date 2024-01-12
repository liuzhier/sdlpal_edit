#pragma once
//使用win DC
#define USE_WIN_FONT

#ifdef _HAS_STD_BYTE
#undef _HAS_STD_BYTE
#endif

#define _HAS_STD_BYTE 0

#include "command.h"
#include "cgl_texture.h"
#include <string>
#include <map>


//using namespace std;

typedef struct SDL_fColor
{
	GLfloat r;
	GLfloat g;
	GLfloat b;
	GLfloat a;
} SDL_fColor;

//本类完成最终显示任务，
class GL_Render
{
public:
	SDL_Window* gpWindow{nullptr};
	CGL_Texture* gpRenderTexture{nullptr};
	CGL_Texture* gpTextTexture{nullptr};//字平面
	int KeepAspectRatio{ 1 };
private:
	SDL_GLContext gpgl_Context = 0;
	//INT glError = 0;

	INT Width, Height;
	GLfloat g_vertices[8];//位置顶点数组
	GLfloat g_texcoord[8];//纹理顶点数组

	GLuint v_verticesID;//顶点ID
	GLuint v_fragmentID;//片断ID
	GLuint v_programID;//GLSL过程ID
	GLint  v_vertexID;	//位置顶点数组ID
	GLint  v_texcoordID; //纹理顶点数组1ID
	GLint  v_texID;
	GLint  v_tex1ID;
	GLint  v_dataID;
	//HDC gpFontDC = NULL;
	//VOID* gp_TextBuf = NULL;
	//HBITMAP gpFontMap = NULL;
	//HFONT gpTextFont = NULL;
	SDL_Renderer* gpSdlRender = NULL;
	PAL_fColor* g_Palette = NULL;
	CGL_Texture* gpFontTexture = NULL;
	CGL_Texture* gpMumberTexture = NULL;
	CGL_Texture* gpASCIITexture = NULL;

	PAL_fColor g_Mave = { 0.0 };
	PAL_fColor g_Ripple = { 0.0 };
	PAL_fColor g_Zoom = { 0.0 };
	PAL_fColor g_Roll = { 0.0 };
	
	class CScript* Pal{};
	
	map <WCHAR, int > FontMap;

public:
	GL_Render(class CScript* Pal,CWnd * win =nullptr);
	~GL_Render();
	/*
	//混合,使用GLSL 实现游戏所需的渲染效果
	//输入1:目标纹理,2：表面1,3：表面2,4：透明度alpha值,5: 模式,6：颜色掩码,7：目标范围,8：源纹理范围
	//模式功能
	//mode = 0.0 混合 透明度， 混合因子
	//mode = 1.0 拷贝后乘颜色乘透明度
	//mode = 2.0 置成单一颜色
	//mode = 3.0 过滤拷贝,全为零的点不拷贝
	//mode = 4.0 与颜色混合，v_mode.y 混合因子
	* 
	VOID Blend_UpdateRender(SDL_Surface* rpText1, SDL_Surface* rpText2 = nullptr,
		WORD rAlpha = 255, const WORD mode = 1, const SDL_Color* rColor = nullptr,
		const SDL_Rect* dstRect = nullptr, const SDL_Rect* srcRect = nullptr);
		*/
	//清除屏幕
	VOID clearScreen(const SDL_Rect* sRect = NULL);;

	VOID clearText(const SDL_Rect* dRect);
	//显示文字UTF8
	VOID PAL_DrawTextUTF8(LPCSTR lpszText, PAL_POS pos, SDL_Color bColor, BOOL fShadow, BOOL fUpdate, int size);
	SIZE PAL_DrawWideText(LPCWSTR lpszText, PAL_POS pos, SDL_Color bColor, BOOL fShadow, BOOL fUpdate, int size);
	int FontInit(string path, INT isBIG5);

	//满屏开关
	VOID PAL_SetFullWindows(BOOL s)
	{
		static int  cw, ch;
		if (s)SDL_GetWindowSize(gpWindow, &cw, &ch);
		SDL_SetWindowFullscreen(gpWindow, s ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
		if (!s)SDL_SetWindowSize(gpWindow, cw, ch);
	};
	VOID setPalette(SDL_Color* sPalette);

private:
	VOID glPreInit();

public:
	VOID RenderBlendCopy(CGL_Texture* rpRender, CGL_Texture* rpText1,
		const WORD rAlpha = 255, const WORD mode = 1, const SDL_Color* rColor = NULL,
		const SDL_Rect* dstRect = NULL, const SDL_Rect* srcRect = NULL);

	VOID RenderBlendCopy(CGL_Texture* rpRender, SDL_Surface* rpSurf,
		const WORD rAlpha = 255, const WORD mode = 1, const SDL_Color* rColor = NULL,
		const SDL_Rect* dstRect = NULL, const SDL_Rect* srcRect = NULL);

	VOID RenderPresent(CGL_Texture* glRender,INT  dAlpha = 255);
	CGL_Texture* creatglTextureFromSurface(SDL_Surface* sSurf);

	SDL_Surface* creatSurfaceFromTexture(const CGL_Texture* text);

	//设置屏幕波动，x 垂直分段，y 水平幅度 ,z 抖动，w 时间 
	inline VOID setMave(GLfloat x, GLfloat y, GLfloat z, GLfloat w) { g_Mave = { x,y,z,w }; }
	//设置水波纹 x y 波纹中心点坐标，z 水波纹宽度，w 起始波纹
	inline VOID setRipple(GLfloat x, GLfloat y, GLfloat z, GLfloat w) { g_Ripple = { x,y,z,w }; }
	//设置缩放 x  y 中心点 z 缩放比率，w  缩放开关
	inline VOID setZoom(GLfloat x, GLfloat y, GLfloat z, GLfloat w) { g_Zoom = { x,y,z,w }; }
	//设置旋转 角度 x 延x轴 y 延y轴 z 延z轴，w  旋转开关
	inline VOID setRoll(GLfloat x, GLfloat y, GLfloat z, GLfloat w) { g_Roll = { x,y,z,w }; }


public:
	INT UpdateTexture(CGL_Texture* rTexture, const SDL_Rect* rRect, const void* pixels, INT pitch);

private:

	inline GLuint loadShader(GLenum shaderType, const GLchar* source)
	{
		int err = 0;
		GLuint shaderId = glCreateShader(shaderType);
		err = glGetError() + err;
		glShaderSource(shaderId, 1, &source, NULL);
		err = glGetError() + err;
		glCompileShader(shaderId);
		err = glGetError() + err;
		GLint result = GL_FALSE;
		glGetShaderiv(shaderId, GL_COMPILE_STATUS, &result);
		err = glGetError() + err;
		//checkShader(shaderId);
		return shaderId;
	}


	inline GLuint compileProgram(GLuint fragmentShaderId, GLuint vertexShaderId)
	{
		GLuint programId = glCreateProgram();
		glAttachShader(programId, vertexShaderId);
		glAttachShader(programId, fragmentShaderId);
		glLinkProgram(programId);
		return programId;
	}

	inline VOID ColorTofColor(const SDL_Color& c, PAL_fColor& a)
	{
		a.r = c.r * Div255;
		a.g = c.g * Div255;
		a.b = c.b * Div255;
		a.a = c.a * Div255;
	}

	//将源和目标范围转成数组
	VOID setRectToArr(const SDL_Rect* src, const SDL_Rect* dst, const SIZE srcSize, const SIZE  dstSize);
private:

};

