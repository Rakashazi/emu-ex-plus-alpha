#pragma once
#include <imagine/gfx/opengl/glIncludes.h>
#include <imagine/base/GLContext.hh>
#include "utils.h"
#include "GLStateCache.hh"
#include <imagine/util/Interpolator.hh>

namespace Gfx
{

extern Base::GLContext gfxContext;
extern GLStateCache glState;

#if !defined CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
static constexpr bool useFixedFunctionPipeline = false;
#else
extern bool useFixedFunctionPipeline;
#endif

extern TimedInterpolator<Gfx::GC> projAngleM;
extern GLfloat maximumAnisotropy, anisotropy, forceAnisotropy;
extern bool useAnisotropicFiltering;
bool usingAutoMipmaping();
extern bool supportBGRPixels;
extern GLenum bgrInternalFormat;
extern bool useCompressedTextures;
extern bool useFBOFuncs;
extern bool useFBOFuncsEXT;
extern bool useVBOFuncs;
extern GLuint globalStreamVBO[4];
extern uint globalStreamVBOIdx;
extern bool useTextureSwizzle;

static constexpr GLuint VATTR_POS = 0, VATTR_TEX_UV = 1, VATTR_COLOR = 2;

TextureHandle newTexRef();
void freeTexRef(TextureHandle texRef);

void initShaders();
GLuint makeProgram(GLuint vShader, GLuint fShader);
bool linkProgram(GLuint program);

extern GLSLProgram *currProgram;
void updateProgramProjectionTransform(GLSLProgram &program);
void updateProgramModelViewTransform(GLSLProgram &program);

void setImgMode(uint mode);

static const bool useGLCache = true;

#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
static void glcMatrixMode(GLenum mode)
{ if(useGLCache) glState.matrixMode(mode); else glMatrixMode(mode); }
#endif
static void glcBindTexture(GLenum target, GLuint texture)
{ if(useGLCache) glState.bindTexture(target, texture); else glBindTexture(target, texture); }
static void glcDeleteTextures(GLsizei n, const GLuint *textures)
{ if(useGLCache) glState.deleteTextures(n, textures); else glDeleteTextures(n, textures); }
static void glcBlendFunc(GLenum sfactor, GLenum dfactor)
{ if(useGLCache) glState.blendFunc(sfactor, dfactor); else glBlendFunc(sfactor, dfactor); }
static void glcBlendEquation(GLenum mode)
{ if(useGLCache) glState.blendEquation(mode); else glBlendEquation(mode); }
static void glcEnable(GLenum cap)
{ if(useGLCache && likely(!glEnableStateHack)) glState.enable(cap); else glEnable(cap); }
static void glcDisable(GLenum cap)
{ if(useGLCache && likely(!glEnableStateHack)) glState.disable(cap); else glDisable(cap); }
static GLboolean glcIsEnabled(GLenum cap)
{
	if(useGLCache && likely(!glEnableStateHack))
		return glState.isEnabled(cap);
	else
		return glIsEnabled(cap);
}
#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
static void glcEnableClientState(GLenum cap)
{ if(useGLCache) glState.enableClientState(cap); else glEnableClientState(cap); }
static void glcDisableClientState(GLenum cap)
{ if(useGLCache) glState.disableClientState(cap); else glDisableClientState(cap); }
static void glcTexEnvi(GLenum target, GLenum pname, GLint param)
{ if(useGLCache) glState.texEnvi(target, pname, param); else glTexEnvi(target, pname, param); }
static void glcTexEnvfv(GLenum target, GLenum pname, const GLfloat *params)
{ if(useGLCache) glState.texEnvfv(target, pname, params); else glTexEnvfv(target, pname, params); }
static void glcColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	if(useGLCache)
		glState.color4f(red, green, blue, alpha);
	else
	{
		glColor4f(red, green, blue, alpha);
		glState.colorState[0] = red; glState.colorState[1] = green; glState.colorState[2] = blue; glState.colorState[3] = alpha; // for color()
	}
}
static void glcTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{ if(useGLCache && likely(!glPointerStateHack)) glState.texCoordPointer(size, type, stride, pointer); else glTexCoordPointer(size, type, stride, pointer); }
static void glcColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{ if(useGLCache && likely(!glPointerStateHack)) glState.colorPointer(size, type, stride, pointer); else glColorPointer(size, type, stride, pointer); }
static void glcVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{ if(useGLCache && likely(!glPointerStateHack)) glState.vertexPointer(size, type, stride, pointer); else glVertexPointer(size, type, stride, pointer); }
#endif
static void glcBindBuffer(GLenum target, GLuint buffer)
{ if(useGLCache) glState.bindBuffer(target, buffer); else glBindBuffer(target, buffer); }
static void glcPixelStorei(GLenum pname, GLint param)
{ if(useGLCache) glState.pixelStorei(pname, param); else glPixelStorei(pname, param); }

}
