#pragma once
#include <imagine/gfx/opengl/gfx-globals.hh>
#include <imagine/base/GLContext.hh>
#include <imagine/gfx/TextureSizeSupport.hh>
#include "utils.h"
#include "GLStateCache.hh"
#include <imagine/util/Interpolator.hh>

namespace Gfx
{

extern Base::GLContext gfxContext;
extern Base::Window *currWin;
extern GLStateCache glState;

#if !defined CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
static constexpr bool useFixedFunctionPipeline = false;
#else
extern bool useFixedFunctionPipeline;
#endif

extern bool checkGLErrors;
extern bool checkGLErrorsVerbose;
extern TimedInterpolator<Gfx::GC> projAngleM;
extern GLfloat maximumAnisotropy, anisotropy, forceAnisotropy;
extern bool useAnisotropicFiltering;
extern bool supportBGRPixels;
extern GLenum bgrInternalFormat;
extern bool useFBOFuncs;
using GenerateMipmapsProto = void (*)(GLenum target);
extern GenerateMipmapsProto generateMipmaps;
extern bool useVBOFuncs;
extern GLuint globalStreamVBO[4];
extern uint globalStreamVBOIdx;
extern bool useTextureSwizzle;
extern bool useUnpackRowLength;
extern bool useSamplerObjects;
extern GLenum luminanceFormat;
extern GLenum luminanceInternalFormat;
extern GLenum luminanceAlphaFormat;
extern GLenum luminanceAlphaInternalFormat;
extern GLenum alphaFormat;
extern GLenum alphaInternalFormat;
extern bool useImmutableTexStorage;
extern TextureSizeSupport textureSizeSupport;
extern bool useLegacyGLSL;

static constexpr GLuint VATTR_POS = 0, VATTR_TEX_UV = 1, VATTR_COLOR = 2;

TextureRef newTex();
void deleteTex(TextureRef texRef);

void initShaders();
GLuint makeProgram(GLuint vShader, GLuint fShader);
bool linkProgram(GLuint program);

extern GLSLProgram *currProgram;
void updateProgramProjectionTransform(GLSLProgram &program);
void updateProgramModelViewTransform(GLSLProgram &program);

void setImgMode(uint mode);

void setActiveTexture(TextureRef tex, uint target);

#ifdef CONFIG_GFX_OPENGL_ES
extern GL_APICALL void (* GL_APIENTRY glGenSamplers) (GLsizei count, GLuint* samplers);
extern GL_APICALL void (* GL_APIENTRY glDeleteSamplers) (GLsizei count, const GLuint* samplers);
extern GL_APICALL GLboolean (* GL_APIENTRY glIsSampler) (GLuint sampler);
extern GL_APICALL void (* GL_APIENTRY glBindSampler) (GLuint unit, GLuint sampler);
extern GL_APICALL void (* GL_APIENTRY glSamplerParameteri) (GLuint sampler, GLenum pname, GLint param);
extern GL_APICALL void (* GL_APIENTRY glTexStorage2D) (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
#endif

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
{ if(useGLCache) glState.enable(cap); else glEnable(cap); }
static void glcDisable(GLenum cap)
{ if(useGLCache) glState.disable(cap); else glDisable(cap); }
static GLboolean glcIsEnabled(GLenum cap)
{
	if(useGLCache)
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
{ if(useGLCache) glState.texCoordPointer(size, type, stride, pointer); else glTexCoordPointer(size, type, stride, pointer); }
static void glcColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{ if(useGLCache) glState.colorPointer(size, type, stride, pointer); else glColorPointer(size, type, stride, pointer); }
static void glcVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{ if(useGLCache) glState.vertexPointer(size, type, stride, pointer); else glVertexPointer(size, type, stride, pointer); }
#endif
#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
static void glcVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer)
{ if(useGLCache) glState.vertexAttribPointer(index, size, type, normalized, stride, pointer); else glVertexAttribPointer(index, size, type, normalized, stride, pointer); }
#endif
static void glcBindBuffer(GLenum target, GLuint buffer)
{ if(useGLCache) glState.bindBuffer(target, buffer); else glBindBuffer(target, buffer); }
static void glcPixelStorei(GLenum pname, GLint param)
{ if(useGLCache) glState.pixelStorei(pname, param); else glPixelStorei(pname, param); }

}
