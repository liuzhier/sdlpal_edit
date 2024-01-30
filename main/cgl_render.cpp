#include "cgl_render.h"
#include <SDL2\SDL_syswm.h>
#include "cscript.h"

#define HZSIZE       32
#define ASCIISIZE    32
const int  TextBufSize = 8;

#define  MastA 0xFF000000
#define  MastB 0x00FF0000
#define  MastG 0x0000FF00
#define  MastR 0x000000FF

//顶点，输入，位置顶点，目标座标，每项四个顶点数据，输出，未经经过转换

//顶点着色器
const static GLchar* const vertexShader =
"#version 130 \n\
in vec2 position;\n\
in vec2 TexCoord;\n\
uniform vec4 v_data[280];\n\
out vec2 sTexCoord;\n\
void main()\n\
{\n\
    vec4 zoom  = v_data[4];\n\
    vec4 roll  = v_data[5];\n\
    gl_Position = vec4(position, 0.0, 1.0) ;\n\
    if(zoom.w > 0.0)//移动缩放\n\
    {\n\
        mat4 m_zoom = mat4(1.0);\n\
        m_zoom[0][0] = zoom.x;\n\
        m_zoom[1][1] = zoom.y;\n\
        m_zoom[2][2] = zoom.z;\n\
        m_zoom[3][3] = zoom.w;\n\
        gl_Position = m_zoom * gl_Position;\n\
    }\n\
    if(roll.w >1.0)//旋转\n\
    {\n\
        float s,c;\n\
        mat4 m_roll = mat4(1.0);//X轴\n\
        m_roll[3][3] = roll.w;\n\
        m_roll[1][1] = m_roll[2][2] = cos(roll.x);\n\
        m_roll[1][2] = sin(roll.x);\n\
        m_roll[2][1] = -sin(roll.x);\n\
        gl_Position = m_roll * gl_Position;\n\
        m_roll = mat4(1.0);//Y轴\n\
        m_roll[0][0] = m_roll[2][2] = cos(roll.y);\n\
        m_roll[0][2] = sin(roll.y);\n\
        m_roll[2][0] = -sin(roll.y);\n\
        gl_Position = m_roll * gl_Position;\n\
        m_roll = mat4(1.0);//Z轴\n\
        m_roll[0][0] = m_roll[1][1] = cos(roll.z);\n\
        m_roll[0][1] = sin(roll.z);\n\
        m_roll[1][0] = -sin(roll.z);\n\
        gl_Position = m_roll * gl_Position;\n\
    }\n\
    sTexCoord = TexCoord ;\n\
}";


//片元
//mode = 0.0 混合 v_mode.y 结果的alpha值
//mode = 1.0 拷贝后乘颜色，乘v_mode.y
//mode = 2.0 置成单一颜色
//mode = 3.0 过滤颜色拷贝，乘混合因子
//mode = 4.0 与单一颜色混合，color颜色
//mode = 6.0 执行字模拷贝，用取得的纹理颜色值乘颜色
//mode = 10.0 执行屏幕翻转显示
// 输入 v_data 0颜色，1.x 模式，1.y alpha 值，
// 2 波动 2.x 2,y x 轴和 y轴 w 幅度 z 时间，
// 3 水波 x,y 中心 w 宽度 z 时间
// 20 起 调色版

const static GLchar* fragmentShader =
"#version 130\n\
uniform vec4 v_data[280];\n\
uniform sampler2D v_tex;\n\
//uniform sampler2D v_tex1;\n\
in vec2 sTexCoord;\n\
out vec4 outColor;\n\
void main()\n\
{\n\
//数组data，20-275,调色版，  0 color，1 mode,\n\
//mode.x ==mode ,mode.y = alpha\n\
//mode.z = 0 纹理32位，1纹理8位，\n\
\n\
vec4 v_color = v_data[0];\n\
vec4 v_mode = v_data[1];\n\
vec4 v_mave = v_data[2];\n\
vec4 v_ripple = v_data[3];\n\
float dx = 0.0 , dy = 0.0;\n\
float pi = 3.1415926;\n\
\n\
if(v_mave.x !=0 || v_mave.y !=0)\n\
{\n\
    float mavex = mod(sTexCoord.y * pi * v_mave.x ,pi * 100.0) ;\n\
    if(v_mave.x != 0.0)dx = sin( mavex + v_mave.w ) * v_mave.z;\n\
    float mavey = mod(sTexCoord.x * pi * v_mave.y ,pi * 100.0) ;\n\
    if(v_mave.y != 0.0)dy = sin( mavey + v_mave.w ) * v_mave.z;\n\
}\n\
else if(v_ripple.z != 0.0 && v_ripple.w != 0.0)\n\
{ \n\
    float distance = distance(sTexCoord, vec2(v_ripple.x, v_ripple.y) );//采样点坐标与中心点的距离\n\
    if ((v_ripple.w - v_ripple.z ) > 0.0 \n\
    && (distance <= (v_ripple.w + v_ripple.z)) \n\
    && (distance >= (v_ripple.w - v_ripple.z))) \n\
    { \n\
    float x = (distance - v_ripple.w); //输入 diff \n\
    //float moveDis =  - pow(8.0  * x, 3.0);//平滑函数 -(8x)^3 采样坐标移动距离 \n\
    float moveDis = 30.0 * x * (x - 0.12)*(x + 0.12);//平滑函数 y=20.0 * x * (x - 0.1)*(x + 0.1)\n\
    vec2 unitDirectionVec = normalize(sTexCoord - vec2(v_ripple.x, v_ripple.y) );//单位方向向量 \n\
    dx = unitDirectionVec.x * moveDis; \n\
    dy = unitDirectionVec.y * moveDis; \n\
    } \n\
    ;\n\
} \n\
\n\
vec2 newcoord = sTexCoord + vec2(dx,dy);\n\
\n\
int m = int(v_mode.z+0.5);\n\
vec4 t0;\n\
\n\
int x = int(v_mode.x +0.001);\n\
if(m == 1 )\n\
{\n\
//纹理为8位带索引，t0 返回颜色值\n\
    int c0 = int(texture(v_tex,newcoord).r * 255.1);\n\
    t0 = v_data[c0 + 20];\n\
    //if(x == 3 && c0 == 0)t0 *= 0.0; \n\
} else\n\
    t0 = texture(v_tex,newcoord);\n\
\n\
\n\
if(x == 0)\n\
//纹理透明，v_mode.y 透明值\n\
    { \n\
    outColor = t0 ;\n\
    outColor.a = v_mode.y;\n\
    }\n\
if(x == 1)\n\
//拷贝后乘Alpha\n\
    {\n\
    outColor = t0;\n\
    //outColor *= v_color;\n\
    outColor *= v_mode.y;\n\
    outColor.a = 1.0;\n\
    }\n\
if(x == 2)\n\
//单一颜色输出\n\
    {\n\
    outColor = v_color;\n\
    outColor *= v_mode.y;\n\
    }\n\
if(x == 3)\n\
//过滤颜色点 \n\
    {\n\
    if((t0.r ==  0.0 && t0.g == 0.0 && t0.b == 0.0 && t0.a == 0.0)  )\n\
    //if(t0 == v_color)\n\
        {\
        //抛弃 \n \
        discard;\n\
        }\n\
    else\n \
        outColor = t0;\n\
    outColor *= v_mode.y;\n\
    outColor.a = 1.0;\n\
}\n\
if(x == 4)\n\
//与单一颜色混合\n\
    { \
    outColor = mix(t0,v_color ,v_mode.y);\n\
    outColor.a = 1.0;\n\
    }\n\
if(x == 6)\n\
//执行字模拷贝，用取得的纹理颜色值乘颜色\n\
    {\n\
    if((t0.r < 0.08  )  )\n\
    {\
        //抛弃 \n \
        discard;\n\
    }\n\
    outColor = v_color;\n\
    //outColor.a = t0.r + (1.0 - t0.r) * 0.8;//透明值\n\
    outColor.a = 1.0; \n\
    }\n\
if(x == 10.0)//屏幕反转\n\
{\n\
    outColor = texture(v_tex,vec2(sTexCoord.x,1.0 - sTexCoord.y));\n\
    outColor *= v_mode.y;\n\
    outColor.a = 1.0;\n\
}\n\
}";


VOID GL_Render::glPreInit()
{
    //要求在主窗口建立之前
    //opengl 3.1
    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    //SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1); //设置多缓存的个数
    //SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

}

//#include <GL/glew.h>

CWnd* win;

GL_Render::GL_Render(class CScript* pal,CWnd * winprt) :
    gpWindow(nullptr),
    gpgl_Context(nullptr),
    gpRenderTexture(nullptr),
    gpTextTexture(nullptr),
    Width(0), Height(0),
    g_vertices(), g_texcoord(),
    v_verticesID(0), v_fragmentID(0), v_programID(0),
    v_vertexID(0), v_texcoordID(0),
    v_texID(0), v_tex1ID(0),
    Pal(pal)
{
    win = winprt;
    
    glPreInit();
    SDL_DisplayMode wMod;
    SDL_GetDesktopDisplayMode(0,&wMod);
    
    Width = wMod.w * 0.6;
    Height = wMod.h * 0.6;
    //创建主窗口
    HDC   dc;
    HGLRC rdc;
    if (!win)
    {
        gpWindow = SDL_CreateWindow("载入 loading......",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, Width, Height,
            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI |
            SDL_WINDOW_HIDDEN);// | SDL_WINDOW_BORDERLESS);// | SDL_WINDOW_ALWAYS_ON_TOP);

        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
        //置opengl 上下文,
        gpgl_Context = SDL_GL_CreateContext(gpWindow);
        if (!gpgl_Context)
            exit(2);
#ifdef GLAD
        //装入glad函数指针
        if (!gladLoadGLLoader(SDL_GL_GetProcAddress))
            exit(2);
#else

#ifdef GLEW
        if (glewInit())
            exit(2);
#endif

#endif
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        SDL_GetWindowWMInfo(gpWindow, &wmInfo);
        HWND hwnd = wmInfo.info.win.window;

        //SDL_ShowWindow(gpWindow);
    }
    else
    {
        //建立RC
        //gpWindow =  SDL_CreateWindowFrom(win);
        PIXELFORMATDESCRIPTOR pfd =
        {
           sizeof(PIXELFORMATDESCRIPTOR),    // pfd结构的大小
           1,                                // 版本号
           PFD_DRAW_TO_WINDOW |              // 支持在窗口中绘图
           PFD_SUPPORT_OPENGL |              // 支持OpenGL
           PFD_DOUBLEBUFFER,                 // 双缓存模式
           PFD_TYPE_RGBA,                    // RGBA 颜色模式
           32,                               // 32 位颜色深度
           0, 0, 0, 0, 0, 0,                 // 忽略颜色位
           0,                                // 没有非透明度缓存
           0,                                // 忽略移位位
           0,                                // 无累加缓存
           0, 0, 0, 0,                       // 忽略累加位
           16,                               // 32 位深度缓存   
           0,                                // 无模板缓存
           0,                                // 无辅助缓存
           PFD_MAIN_PLANE,                   // 主层
           0,                                // 保留
           0, 0, 0                           // 忽略层,可见性和损毁掩模
        };

        dc = win->GetWindowDC()->GetSafeHdc();
        int nPixelFormat   // 像素点格式
            = ChoosePixelFormat(dc, &pfd);
        SetPixelFormat(dc, nPixelFormat, &pfd);
        rdc = wglCreateContext(dc);
        wglMakeCurrent(dc, rdc);
        if (gladLoadGL() == NULL)
            exit(14);
        ASSERT(wglGetCurrentDC() == dc);
        ASSERT(wglGetCurrentContext() == rdc);
    }
    Width = WindowWidth;
    Height = WindowHeight;

    int err = 0;
    v_verticesID = loadShader(GL_VERTEX_SHADER, vertexShader);
    err = glGetError() | err;
    v_fragmentID = loadShader(GL_FRAGMENT_SHADER, fragmentShader);
    err = glGetError() | err;
    v_programID = compileProgram(v_verticesID, v_fragmentID);
    err = glGetError() | err;

    v_texID = glGetUniformLocation(v_programID, "v_tex");
    v_dataID = glGetUniformLocation(v_programID, "v_data");
    v_texcoordID = glGetAttribLocation(v_programID, "TexCoord");
    v_vertexID = glGetAttribLocation(v_programID, "position");

    gpRenderTexture = new CGL_Texture(RealWidth, RealHeight, 32);
}

    GL_Render::~GL_Render()
    {
        delete gpRenderTexture;
        //清理
        if (g_Palette)
            delete[] g_Palette;
        if (gpgl_Context)
            SDL_GL_DeleteContext(gpgl_Context);
        if (gpSdlRender)
            SDL_DestroyRenderer(gpSdlRender);
        if (gpWindow)
            SDL_DestroyWindow(gpWindow);
        if (gpFontTexture)
            delete gpFontTexture;
        if (gpASCIITexture)
            delete gpASCIITexture;

        gpRenderTexture = nullptr;
        gpgl_Context = nullptr;
        gpSdlRender = nullptr;
        gpWindow = nullptr;
        g_Palette = nullptr;
        gpTextTexture = nullptr;
    } 


#ifndef max
#define max(a, b)    (((a) > (b)) ? (a) : (b))
#endif
#define fabs(a) ((a) > 0 ? (a):(-(a)) )


CGL_Texture* GL_Render::creatglTextureFromSurface(SDL_Surface* sSurf)
////返回GL纹理
{  
    //根据表面新建纹理
    CGL_Texture* m_Texture;
    if (sSurf->format->BytesPerPixel != 1 && 
        sSurf->format->BytesPerPixel != 4 &&sSurf->format->BytesPerPixel != 2)
    {
        //特殊格式表面
        SDL_Surface* m_surf = SDL_ConvertSurfaceFormat(sSurf, SDL_PIXELFORMAT_ABGR8888, 0);
        m_Texture = new CGL_Texture(m_surf->w, m_surf->h, m_surf->format->BitsPerPixel);
        UpdateTexture(m_Texture, NULL, m_surf->pixels, m_surf->pitch);
        SDL_FreeSurface(m_surf);
        return m_Texture;
    }
    
    if (sSurf->format->palette)
        m_Texture = new CGL_Texture(sSurf->w, sSurf->h, sSurf->format->BitsPerPixel,
            sSurf->format->palette->colors);
    else
        m_Texture = new CGL_Texture(sSurf->w, sSurf->h, sSurf->format->BitsPerPixel,
            nullptr);

    UpdateTexture(m_Texture, NULL, sSurf->pixels, sSurf->pitch);
    return m_Texture;
}

//
//根据纹理创建SDL表面
// 输入：纹理
// 输出：320*200 32位RDBA格式表面
// 出错返回空指针
//
SDL_Surface* GL_Render::creatSurfaceFromTexture(const CGL_Texture* text)
{
    int err;
    Uint8* tbuf;
    if (!text)return nullptr;
    CGL_Texture* mText = new CGL_Texture(320, 200, 32);
    RenderBlendCopy(mText, (CGL_Texture*)text);
    if (mText->genFBOID == 0)
        glGenFramebuffers(1, &mText->genFBOID);
    glBindFramebuffer(GL_FRAMEBUFFER, mText->genFBOID);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D, mText->genID, 0);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    err = (status != GL_FRAMEBUFFER_COMPLETE);
    err = glGetError();
    //
    GLuint bufId;
    glGenBuffers(1, &bufId);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, bufId);
    glBufferData(GL_PIXEL_PACK_BUFFER, 320 * 200 * 4, NULL, GL_STREAM_COPY);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    err = glGetError();
    glReadPixels(0, 0, 320, 200, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    err = glGetError();
    tbuf = (Uint8*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_WRITE);

    SDL_Surface * rsurf = SDL_CreateRGBSurface(0, 320, 200, 32, MastR, MastG, MastB, MastA);
    memcpy(rsurf->pixels, tbuf, 320 * 200 * 4);
    //清理;
    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    glBindBuffer(GL_TEXTURE, 0);
    glDeleteBuffers(1,&bufId);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    delete mText;
    //delete []tbuf;
    return rsurf;
}

INT  GL_Render::UpdateTexture(CGL_Texture* rTexture, const SDL_Rect* rRect, const void* pixels, INT pitch)
{
    // = SDL_UpdateTexture;
    SDL_Rect sRect = { 0,0,rTexture->w,rTexture->h };
    if (rRect == NULL)
        rRect = &sRect;
    glEnable(GL_TEXTURE_2D);
    //绑定纹理
    glBindTexture(GL_TEXTURE_2D, rTexture->genID);
    //绑定BUF
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, rTexture->genPBOID);
    //申请一个空间操作
    //分配空间,载入数据
    glBufferData(GL_PIXEL_UNPACK_BUFFER,
        rRect->h * pitch,
        pixels, GL_STREAM_DRAW);
    int m_format;
    int m_type;
    switch (rTexture->BitsPerPixel)
    {
    case 8:
        m_type = GL_UNSIGNED_BYTE;
        m_format = GL_RED;
        break;
    case 16:
    case 15:
        m_type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
        //m_type = GL_UNSIGNED_SHORT_5_5_5_1;
        m_format = GL_BGRA;
        break;
    case 32:
    default:
        m_type = GL_UNSIGNED_BYTE;
        m_format = GL_RGBA;
        break;
    }

    glTexSubImage2D(GL_TEXTURE_2D, 0, rRect->x, rRect->y, rRect->w,
        rRect->h, m_format, m_type, NULL);//最后一个参数为空，申请空间

    glGenerateMipmap(GL_TEXTURE_2D);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);//恢复之间的缺省绑定
    glBindTexture(GL_TEXTURE_2D, 0);//撤消绑定
    glDisable(GL_TEXTURE_2D);
    return 0;
}

//将源和目标范围转成数组

VOID GL_Render::setRectToArr(const SDL_Rect* src, const SDL_Rect* dst, const SIZE srcSize, const SIZE dstSize)
{
    //转换成数组
    GLfloat minx, miny, maxx, maxy;
    GLfloat minu, maxu, minv, maxv;
    if (dst && dst->w && dst->h && dstSize.cx && dstSize.cy)
    {
        minx = (GLfloat)dst->x / (GLfloat)dstSize.cx * 2 - 1.0;
        maxx = (GLfloat)(dst->x + dst->w) / (GLfloat)dstSize.cx * 2 - 1.0;
        miny = (GLfloat)dst->y / dstSize.cy * 2 - 1.0;
        maxy = (GLfloat)(dst->y + dst->h) / (GLfloat)dstSize.cy * 2 - 1.0;
    }
    else
    {
        minx = -1.0f;
        maxx = 1.0f;
        miny = -1.0f;
        maxy = 1.0f;
    }

    if (src && src->w && src->h && srcSize.cx && srcSize.cy)
    {
        minu = ((GLfloat)src->x / (GLfloat)srcSize.cx);
        maxu = ((GLfloat)src->x + src->w) / (GLfloat)srcSize.cx;
        minv = ((GLfloat)src->y / (GLfloat)srcSize.cy);
        maxv = ((GLfloat)(src->y + src->h)) / (GLfloat)srcSize.cy;
    }
    else
    {
        minu = 0.0f;
        maxu = 1.0f;
        minv = 0.0f;
        maxv = 1.0f;
    }

    g_vertices[0] = minx;
    g_vertices[1] = miny;
    g_vertices[2] = maxx;
    g_vertices[3] = miny;
    g_vertices[4] = maxx;
    g_vertices[5] = maxy;
    g_vertices[6] = minx;
    g_vertices[7] = maxy;

    g_texcoord[0] = minu;
    g_texcoord[1] = minv;
    g_texcoord[2] = maxu;
    g_texcoord[3] = minv;
    g_texcoord[4] = maxu;
    g_texcoord[5] = maxv;
    g_texcoord[6] = minu;
    g_texcoord[7] = maxv;

}



VOID  GL_Render::setPalette(SDL_Color * sPalette)
{
    if (!g_Palette)
        g_Palette = new PAL_fColor[256];
    for (int n = 0; n < 256; n++)
    {
        ColorTofColor(sPalette[n], g_Palette[n]);
    }
}



VOID GL_Render::RenderBlendCopy(
    CGL_Texture* rpRender,
    SDL_Surface* rpSurf,
    const WORD rAlpha,
    const WORD mode,
    const SDL_Color* rColor,
    const SDL_Rect* dstRect,
    const SDL_Rect* srcRect
)
{
    CGL_Texture* dpText = NULL;
    if (rpSurf)
    {
        dpText = creatglTextureFromSurface(rpSurf);
    }
    RenderBlendCopy(rpRender, dpText, rAlpha, mode, rColor, dstRect, srcRect);
    if (dpText) delete dpText;
}


VOID GL_Render::RenderBlendCopy(
    CGL_Texture* rpRender,
    CGL_Texture* rpText1,
    const WORD rAlpha,
    const WORD mode,
    const SDL_Color* rColor,
    const SDL_Rect* dstRect,
    const SDL_Rect* srcRect
)
/*
模式功能
//mode = 0.0 混合 v_mode.y 混合因子
//mode = 1.0 拷贝后乘颜色
//mode = 2.0 置成单一颜色
//mode = 3.0 过滤拷贝全为零的点不拷贝
//mode = 4.0 与颜色混合，v_mode.y 混合因子
//mode = 6.0 显示字模
//mode = 10.0 实现显示反转
* */
{
    SIZE msSize = { PictureWidth,PictureHeight };
    int err = 0;
    if (rpText1)
    {
        msSize = { rpText1->w,rpText1->h };
    }

    SIZE mdSize = { RealWidth,RealHeight };
    if (rpRender)
    {
        mdSize = { rpRender->w,rpRender->h };
        glViewport(0, 0, mdSize.cx, mdSize.cy);
        //绑定缓存区
        if (rpRender->genFBOID <= 0)
            glGenFramebuffers(1, &rpRender->genFBOID);
        glBindFramebuffer(GL_FRAMEBUFFER, rpRender->genFBOID);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, rpRender->genID, 0);
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        err = (status != GL_FRAMEBUFFER_COMPLETE);
        err = glGetError() + err;
    }
    else
    {
        //渲染到屏幕，随窗口改变大小
        static SDL_Rect zView = { 0,0,0,0 };
        static SDL_Rect oldView = zView;

        if (gpWindow)
            SDL_GetWindowSize(gpWindow, &zView.w, &zView.h);
        else
        {
            CRect rect;
            GetWindowRect(win->GetSafeHwnd(), &rect);
            zView.w = rect.Width();
            zView.h = rect.Height();
        }
        zView.x = zView.y = 0;
        if (zView.w != oldView.w || zView.h != oldView.h)
        {
            zView.h = max(RealHeight * 0.25, zView.h);
            zView.w = max(RealWidth * 0.25, zView.w);
            if (gpWindow)
                SDL_GetWindowSize(gpWindow, &zView.w, &zView.h);
            else
            {
                CRect rect;
                GetWindowRect(win->GetSafeHwnd(), &rect);
                zView.w = rect.Width();
                zView.h = rect.Height();
            }
            oldView = zView;
        }
        // 绑定默认FBO（窗体帧缓冲区的ID是0）
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (gpSdlRender)
        {
            SDL_SetRenderDrawColor(gpSdlRender, 0, 0, 0, 0);
            SDL_RenderClear(gpSdlRender);
        }
        zView = { 0,0,0,0 };
        //保持图片显示比例
        if (gpWindow)
            SDL_GetWindowSize(gpWindow, &zView.w, &zView.h);
        else
        {
            CRect rect;
            GetWindowRect(win->GetSafeHwnd(), &rect);
            zView.w = rect.Width();
            zView.h = rect.Height();
        }

        if ( KeepAspectRatio && (fabs((double)zView.w / zView.h - 1.6) > 0.02))
        {
            double ra = (double)zView.w / zView.h - 1.6;
            if (ra > 0)
            {
                zView.x = (int)((zView.w - zView.h * 1.6) / 2.0);
                zView.w -= zView.x * 2;
            }
            else
            {
                zView.y = (zView.h - zView.w / 1.6) / 2;
                zView.h -= zView.y * 2;
            }
        }
        glViewport(zView.x, zView.y, zView.w, zView.h);
    }

    setRectToArr(srcRect, dstRect, msSize, mdSize);

    SDL_Color vColor = { 255,255,255,255 };
    if (rColor == NULL)
        rColor = &vColor;
    
    if (rpText1)
    {
        glEnable(GL_TEXTURE_2D);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, rpText1->genID);
        err = glGetError() + err;
    }
    err = glGetError() + err;
    glUseProgram(v_programID);
    glEnableVertexAttribArray(v_vertexID);
    glEnableVertexAttribArray(v_texcoordID);

    err = glGetError() + err;

    glVertexAttribPointer(v_vertexID, 2, GL_FLOAT, GL_FALSE, 0, g_vertices);
    glVertexAttribPointer(v_texcoordID, 2, GL_FLOAT, GL_FALSE, 0, g_texcoord);
    err = glGetError() + err;
    PAL_fColor dataBuf[280]{0};
    int cBit = 0;
    //检查RENDER的分辨率
    if (!rpRender || rpRender->BitsPerPixel == 32)
    {
        //检查纹理1的分辨率
        if (rpText1 && rpText1->BitsPerPixel == 8 && rpText1->gfPalette)
        {
            cBit |= 1;
            SDL_memcpy(dataBuf+20, rpText1->gfPalette, sizeof(PAL_fColor) * 256);
        }
    }
    //
    dataBuf[0] = { (GLfloat)(rColor->r * Div255),(GLfloat)(rColor->g * Div255)
        ,(GLfloat)(rColor->b * Div255),(GLfloat)(rColor->a * Div255) };
    dataBuf[1] = { (GLfloat)(mode), (GLfloat)(rAlpha * Div255) ,(GLfloat)cBit,0.0 };
    dataBuf[2] = g_Mave;
    dataBuf[3] = g_Ripple;
    dataBuf[4] = g_Zoom;
    dataBuf[5] = g_Roll;

    glUniform4fv(v_dataID, 280, (const float*)dataBuf);

    if ((mode == 0 || mode == 6) && 1)
    {
        glUniform1i(v_texID, 0);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//设置融合函数
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glDisable(GL_BLEND);
    }
    else 
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    err = glGetError() + err;

    glDisableVertexAttribArray(v_vertexID);
    glDisableVertexAttribArray(v_texcoordID);
 
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    err = glGetError() + err;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);
    glDisable(GL_TEXTURE_2D);
    err = glGetError() + err;
}

VOID GL_Render::RenderPresent(CGL_Texture* glRender,INT dAlpha)
{
    RenderBlendCopy(NULL, glRender, dAlpha, 10);
    if (gpWindow)
    {
        SDL_ShowWindow(gpWindow);
        SDL_GL_SwapWindow(gpWindow);
    }
    else
    {
        glFlush();
        auto s = wglGetCurrentDC();
        SwapBuffers(s);
        
    }
    //SDL_RenderPresent(gpSdlRender);
}



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

VOID GL_Render::clearScreen(const SDL_Rect* sRect)
{
    clearText(sRect);

}

VOID GL_Render::clearText(const SDL_Rect* sRect)
{
    SDL_Rect dstRect = { 0,0,RealWidth,RealHeight };
    if (sRect)
        dstRect = { (int)(sRect->x * PictureRatio),
        (int)(sRect->y * PictureRatio),(int)(sRect->w * PictureRatio),(int)(sRect->h * PictureRatio) };

    SDL_Color rcolor = { 0,0,0,0 };
}

VOID GL_Render::PAL_DrawTextUTF8(LPCSTR lpszText, PAL_POS pos, SDL_Color bColor, BOOL fShadow, BOOL fUpdate, int size)
{
    //功能：在文字缓存区写字符串
    int len = MultiByteToWideChar(CP_UTF8, 0, lpszText, -1, NULL, 0);
    LPWSTR lpszTextR = new wchar_t[len];
    MultiByteToWideChar(CP_UTF8, 0, lpszText, -1, lpszTextR, len);
    PAL_DrawWideText(lpszTextR, pos, bColor, fShadow, fUpdate, size);
    delete [] lpszTextR;
}

SIZE GL_Render::PAL_DrawWideText(LPCWSTR lpszTextR, PAL_POS pos, SDL_Color bColor, BOOL fShadow, BOOL fUpdate, int size)
{
    //功能：在屏幕上打印宽字符串
    //返回：无
     
    int cx = PAL_X(pos) * PictureRatio;
    int cy = PAL_Y(pos) * PictureRatio;

    int len = lstrlen(lpszTextR);
    if (!size)size = 16;
    size *= PictureRatio;
    SIZE wsz = { 0,size };
    SIZE zsz = { size,size };
    SDL_Rect srcRect;
    SDL_Rect dstRect;
    CGL_Texture* srcTexture;
    for (int n = 0; n < len; n++)
    {
        if (isascii(lpszTextR[n]))
        {
            if (lpszTextR[n] < 32)
                continue;
            zsz = { size / 2,size };
            srcTexture = gpASCIITexture;
            srcRect = { (lpszTextR[n] - 32) * ASCIISIZE ,0,(int)((DOUBLE)ASCIISIZE * 0.6) ,ASCIISIZE };
            dstRect = { cx,cy,zsz.cx,zsz.cy };
            cx += zsz.cx;
            wsz.cx += zsz.cx;
        }
        else
        {
            zsz = {size,size};
            srcTexture = gpFontTexture;
            WCHAR s = lpszTextR[n];
            map<WCHAR, INT> ::iterator iter;
            iter = FontMap.find(s);
            if (iter == FontMap.end())
                //no find
                continue;
            int off = iter->second;
            srcRect = { (off & 63) * HZSIZE,(off >> 6) * HZSIZE,HZSIZE,HZSIZE };
            dstRect = { cx,cy,zsz.cx,zsz.cy };
            cx += zsz.cx;
            wsz.cx += zsz.cx;
        }
        if (fShadow)
        {
            SDL_Color mColor = { 2,2,2,0 };
            SDL_Rect mdstRect = dstRect;
            mdstRect.x++;
            RenderBlendCopy(gpTextTexture, srcTexture, 255, 6, &mColor,
                &mdstRect, &srcRect);
            mdstRect.y++;
            RenderBlendCopy(gpTextTexture, srcTexture, 255, 6, &mColor,
                &mdstRect, &srcRect);
        }
        if (bColor.r == 0 && bColor.g == 0 && bColor.b == 0)
            bColor = { 4,4,4,128 };
        RenderBlendCopy(gpTextTexture, srcTexture, 255, 6, &bColor,
            &dstRect, &srcRect);
    }
    //到这里，已经完成字符串.
    return wsz;

}



static void*
StartDrawToBitmap(HDC hdc, HBITMAP* hhbm, int width, int height)
{
    BITMAPINFO info;
    BITMAPINFOHEADER* infoHeader = &info.bmiHeader;
    BYTE* bits = NULL;
    if (hhbm) {
        SDL_zero(info);
        infoHeader->biSize = sizeof(BITMAPINFOHEADER);
        infoHeader->biWidth = width;
        infoHeader->biHeight = -1 * SDL_abs(height);
        infoHeader->biPlanes = 1;
        infoHeader->biBitCount = 32;
        infoHeader->biCompression = BI_RGB;
        *hhbm = CreateDIBSection(hdc, &info, DIB_RGB_COLORS, (void**)&bits, 0, 0);
        if (*hhbm)
            SelectObject(hdc, *hhbm);
    }
    return bits;
}

static void
StopDrawToBitmap(HDC hdc, HBITMAP* hhbm)
{
    if (hhbm && *hhbm) {
        DeleteObject(*hhbm);
        *hhbm = NULL;
    }
}


//使用win dc 创建字库
int GL_Render::FontInit(string path, INT  fUseBIG5)
//功能初始化字模纹理
//输入，游戏文件所在目录
//使用字库情况，第一个字节 字库简体还是繁体，第二个字节 游戏是简体还是繁体，
//字库全路径文件名
//返回：0 成功
{
    VOID* gpTextBuf = NULL;
    LPCWSTR sFontList[]//0 宋体，1 仿宋体 ，2 黑体 3 幼圆体 ，4 楷体
    {
        L"宋体",L"仿宋体",L"黑体",L"幼圆体",L"楷体",
    };
    FILE* fp;
    LPCSTR name[4] = { "wor16.asc","word.dat","m.msg" ,"desc.dat" };
    size_t nChar = 0;
    for (int n = 0; n <= 3; n++)
    {
        string filename = path + name[n];
        if ((fopen_s(&fp, filename.c_str(), "rb")))
        {
            if (n == 0 || n == 3)
                continue;//不存在，跳过
            else
                return -1;//出错
        }

        UINT8 s[3] = { 0 };
        int count = 0;
        for (; !feof(fp);)
        {
            s[0] = fgetc(fp);
            size_t i = ftell(fp);
            if (s[0] < 127)
                continue;
            s[1] = fgetc(fp);
            WCHAR wstr[2] = { 0 };
            WCHAR rewstr[2] = { 0 };

            switch (fUseBIG5)
            {
                //     //游戏+字库
            case 0://简体+简体
                MultiByteToWideChar(936, 0, (LPCSTR)s, -1, rewstr, 1);
                break;
            case 1://繁体+简体
            {
                MultiByteToWideChar(950, 0, (LPCSTR)s, -1, wstr, 1);
                wstr[1] = 0;
                //转成简体字
                DWORD wLCID = MAKELCID(MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED), SORT_CHINESE_BIG5);
                LCMapString(wLCID, LCMAP_SIMPLIFIED_CHINESE, wstr, 1, rewstr, 1);
            }
            break;
            case 2://简体+繁体
            {
                MultiByteToWideChar(936, 0, (LPCSTR)s, -1, wstr, 1);
                wstr[1] = 0;
                //转成繁体字
                DWORD wLCID = MAKELCID(MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED), SORT_CHINESE_PRC);
                LCMapString(wLCID, LCMAP_TRADITIONAL_CHINESE, wstr, 1, rewstr, 1);
            }
            break;
            case 3://繁体+繁体
                MultiByteToWideChar(950, 0, (LPCSTR)s, -1, rewstr, 1);
                break;
            default:
                break;
            }
            //将字符插入字典
            rewstr[1] = 0;
            if (FontMap.count(rewstr[0]))
                continue;
            FontMap.insert(pair<WCHAR, INT>(rewstr[0], FontMap.size()));
        }
        ::fclose(fp);
    }
    //////////
    {//汉字字典
        HBITMAP gpFontMap;
        //将字典中的字生成纹理
        HDC gpFontDC = CreateCompatibleDC(GetDC(NULL));
        HFONT gpTextFont{};

        gpTextFont = CreateFont(HZSIZE, 0, 0, 0, 
            Pal->gConfig->m_Function_Set[46] * 100, 0, 0, 0, 
            Pal->gConfig->fIsUseBig5 ? CHINESEBIG5_CHARSET:DEFAULT_CHARSET
            //CHINESEBIG5_CHARSET
            , 0, 0, 0, 0, sFontList[Pal->gConfig->m_Function_Set[45]]);
        //文字位图背景颜色
        SetBkColor(gpFontDC, RGB(0, 0, 0));
        //文字位图

        //建立文字缓存
        nChar = FontMap.size();
        void* gpTextBuf = StartDrawToBitmap(gpFontDC, &gpFontMap, 64 * HZSIZE,
            (HZSIZE * (nChar / 64 + 1)));
        //将字典写到纹理中
        for (map<WCHAR, INT> ::iterator iter = FontMap.begin();
            iter != FontMap.end(); iter++)
        {
            int n = iter->second;
            WCHAR rewstr[2] = { iter->first,0 };
            size_t cx = (size_t)(n & 63) * HZSIZE;
            size_t cy = (size_t)(n >> 6) * HZSIZE;
            SelectObject(gpFontDC, gpTextFont);
            SetTextColor(gpFontDC, RGB(0xff, 0xff, 0xff));
            TextOut(gpFontDC, cx, cy, rewstr, 1);
        }
        //生成纹理
        //复制到纹理
        SDL_Surface* fontsurf = SDL_CreateRGBSurfaceWithFormatFrom(gpTextBuf, HZSIZE * 64,
            (nChar / 64 + 1) * HZSIZE, 32,
            HZSIZE * 64 * sizeof(INT32), SDL_PIXELFORMAT_RGBA8888);
        //保存字符图
        SDL_SaveBMP(fontsurf, "fontt.bmp");
        gpFontTexture = creatglTextureFromSurface(fontsurf);
        SDL_FreeSurface(fontsurf);
        //delete[]gpTextBuf;

        //清理
        StopDrawToBitmap(gpFontDC, &gpFontMap);
        DeleteObject(gpTextFont);
    }
    //////
   
    {//英文
        HBITMAP gpFontMap{};
        //将字典中的字生成纹理
        HDC gpFontDC = CreateCompatibleDC(GetDC(NULL));
        HFONT gpAscFont = CreateFont(ASCIISIZE, 0, 0, 0, 300, 0, 0, 0, DEFAULT_CHARSET
            , 0, 0, 0, 0, L"黑体");
        //文字位图背景颜色
        SetBkColor(gpFontDC, RGB(0, 0, 0));
        //文字位图

        //建立文字缓存
        void* gpTextBuf = StartDrawToBitmap(gpFontDC, &gpFontMap, 96 * ASCIISIZE , ASCIISIZE);
        //将字典写到纹理中
        for (WCHAR n = 0; n < 96; n++)
        {
            WCHAR s[2]{ n + 32,0 };
            size_t cx = n * ASCIISIZE;
            SelectObject(gpFontDC, gpAscFont);
            SetTextColor(gpFontDC, RGB(0xff, 0xff, 0xff));
            TextOut(gpFontDC, cx, 0, s, 1);
        }
        //复制到纹理
        SDL_Surface* asciiSurf = SDL_CreateRGBSurfaceWithFormatFrom(gpTextBuf, ASCIISIZE * 96,
            ASCIISIZE, 32, ASCIISIZE * 96 * sizeof(INT32), SDL_PIXELFORMAT_RGBA8888);
        //保存字符图
        //SDL_SaveBMP(asciiSurf, "ascii.bmp");
        gpASCIITexture = creatglTextureFromSurface(asciiSurf);
        SDL_FreeSurface(asciiSurf);
        //清理
        StopDrawToBitmap(gpFontDC, &gpFontMap);
        DeleteObject(gpAscFont);
    }
    
    return 0;
}


