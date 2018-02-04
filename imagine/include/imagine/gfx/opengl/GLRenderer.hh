#pragma once

/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/config/defs.hh>
#include <imagine/base/GLContext.hh>
#include <imagine/base/Timer.hh>
#include <imagine/gfx/defs.hh>
#include <imagine/gfx/TextureSizeSupport.hh>
#include <imagine/gfx/Texture.hh>
#include <imagine/gfx/opengl/GLStateCache.hh>
#include <imagine/util/Interpolator.hh>

namespace Gfx
{

#ifndef GL_KHR_debug
typedef void (GL_APIENTRY *GLDEBUGPROCKHR)(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar*, const void *);
using GLDEBUGPROC = GLDEBUGPROCKHR;
#define GL_DEBUG_OUTPUT_KHR 0x92E0
#define GL_DEBUG_OUTPUT 0x92E0
#endif

class DrawContextSupport
{
public:
	bool isConfigured = false;
	#ifndef CONFIG_GFX_OPENGL_ES
	bool hasAnisotropicFiltering = false;
	bool hasMultisample = false;
	bool hasMultisampleHints = false;
	#endif
	bool hasBGRPixels = false;
	GLenum bgrInternalFormat = GL_BGRA;
	bool hasVBOFuncs = false;
	bool hasTextureSwizzle = false;
	bool hasUnpackRowLength = !Config::Gfx::OPENGL_ES;
	bool hasSamplerObjects = !Config::Gfx::OPENGL_ES;
	bool hasImmutableTexStorage = false;
	bool hasPBOFuncs = false;
	bool shouldSpecifyDrawReadBuffers = false;
	bool hasDebugOutput = false;
	bool useLegacyGLSL = Config::Gfx::OPENGL_ES;
	#ifdef __ANDROID__
	bool hasEGLImages = false;
	bool hasExternalEGLImages = false;
	#endif
	#if !defined CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	static constexpr bool useFixedFunctionPipeline = false;
	#else
	bool useFixedFunctionPipeline = true;
	#endif
	TextureSizeSupport textureSizeSupport{};
	#ifdef CONFIG_GFX_OPENGL_ES
	void (* GL_APIENTRY glGenSamplers) (GLsizei count, GLuint* samplers){};
	void (* GL_APIENTRY glDeleteSamplers) (GLsizei count, const GLuint* samplers){};
	void (* GL_APIENTRY glBindSampler) (GLuint unit, GLuint sampler){};
	void (* GL_APIENTRY glSamplerParameteri) (GLuint sampler, GLenum pname, GLint param){};
	void (* GL_APIENTRY glTexStorage2D) (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height){};
	GLvoid* (* GL_APIENTRY glMapBufferRange) (GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access){};
	using UnmapBufferProto = GLboolean (* GL_APIENTRY) (GLenum target);
	UnmapBufferProto glUnmapBuffer{};
	void (* GL_APIENTRY glDrawBuffers) (GLsizei size, const GLenum *bufs){};
	void (* GL_APIENTRY glReadBuffer) (GLenum src){};
	#else
	static void glGenSamplers(GLsizei count, GLuint* samplers) { ::glGenSamplers(count, samplers); };
	static void glDeleteSamplers(GLsizei count, const GLuint* samplers) { ::glDeleteSamplers(count,samplers); };
	static void glBindSampler(GLuint unit, GLuint sampler) { ::glBindSampler(unit, sampler); };
	static void glSamplerParameteri(GLuint sampler, GLenum pname, GLint param) { ::glSamplerParameteri(sampler, pname, param); };
	static void glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) { ::glTexStorage2D(target, levels, internalformat, width, height); };
	static GLvoid* glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) { return ::glMapBufferRange(target, offset, length, access); };
	static GLboolean glUnmapBuffer(GLenum target) { return ::glUnmapBuffer(target); }
	static void glDrawBuffers(GLsizei size, const GLenum *bufs) { ::glDrawBuffers(size, bufs); };
	static void glReadBuffer(GLenum src) { ::glReadBuffer(src); };
	#endif
	GLenum luminanceFormat = GL_LUMINANCE;
	GLenum luminanceInternalFormat = GL_LUMINANCE8;
	GLenum luminanceAlphaFormat = GL_LUMINANCE_ALPHA;
	GLenum luminanceAlphaInternalFormat = GL_LUMINANCE8_ALPHA8;
	GLenum alphaFormat = GL_ALPHA;
	GLenum alphaInternalFormat = GL_ALPHA8;
	#ifdef CONFIG_GFX_OPENGL_ES
	void GL_APIENTRY (*glDebugMessageCallback)(GLDEBUGPROCKHR callback, const void *userParam){};
	static constexpr auto DEBUG_OUTPUT = GL_DEBUG_OUTPUT_KHR;
	#else
	void GL_APIENTRY (*glDebugMessageCallback)(GLDEBUGPROC callback, const void *userParam){};
	static constexpr auto DEBUG_OUTPUT = GL_DEBUG_OUTPUT;
	#endif
	#if defined CONFIG_GFX_OPENGL_ES && CONFIG_GFX_OPENGL_ES_MAJOR_VERSION > 1
	static void generateMipmaps(GLenum target) { ::glGenerateMipmap(target); };
	#else
	using GenerateMipmapsProto = void (* GL_APIENTRY)(GLenum target);
	GenerateMipmapsProto generateMipmaps{}; // set via extensions
	#endif
};

class GLRenderer
{
public:
	Base::GLDisplay glDpy{};
	Base::GLContext gfxContext{};
	Base::GLDrawable currWin{};
	Base::GLBufferConfig gfxBufferConfig{};
	#ifndef CONFIG_GFX_OPENGL_ES
	GLuint streamVAO = 0;
	GLuint streamVBO[6]{};
	uint streamVBOIdx = 0;
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	GLSLProgram *currProgram{};
	GLuint defaultVShader = 0;
	Mat4 modelMat, projectionMat;
	uint modelMatAge = 0, projectionMatAge = 0;
	uint currentVtxArrayPointerID = 0;
	#endif
	Viewport currViewport;
	Angle projectionMatRot = 0;
	Mat4 projectionMatPreTransformed;
	GLuint samplerNames = 0; // used when separate sampler objects not supported
	GLTextureSampler currSampler{};
	TextureSampler defaultClampSampler{};
	TextureSampler defaultNearestMipClampSampler{};
	TextureSampler defaultNoMipClampSampler{};
	TextureSampler defaultNoLinearNoMipClampSampler{};
	TextureSampler defaultRepeatSampler{};
	TextureSampler defaultNearestMipRepeatSampler{};
	ColorComp vColor[4]{}; // color when using shader pipeline
	ColorComp texEnvColor[4]{}; // color when using shader pipeline
	static constexpr uint TEXTURE_PBOS = 1;
	GLuint texturePBO[TEXTURE_PBOS]{};
	uint texturePBOIdx = 0;
	uint usedTexturePBOs = 0;
	Base::Timer releaseShaderCompilerTimer;
	TimedInterpolator<Gfx::GC> projAngleM;
	GLStateCache glState{};
	DrawContextSupport support{};

	GLRenderer() {}
	void initVBOs();
	GLuint getVBO();
	void setupAnisotropicFiltering();
	void setupMultisample();
	void setupMultisampleHints();
	void setupNonPow2Textures();
	void setupNonPow2MipmapTextures();
	void setupNonPow2MipmapRepeatTextures();
	void setupBGRPixelSupport();
	void setupFBOFuncs(bool &useFBOFuncs);
	void setupVAOFuncs();
	void setupTextureSwizzle();
	void setupImmutableTexStorage(bool extSuffix);
	void setupRGFormats();
	void setupSamplerObjects();
	void setupPBO();
	void setupSpecifyDrawReadBuffers();
	void setupUnmapBufferFunc();
	void checkExtensionString(const char *extStr, bool &useFBOFuncs);
	void checkFullExtensionString(const char *fullExtStr);
	void initTexturePBO();
	void discardTexturePBO();
	void verifyCurrentContext();
	void updateProgramProjectionTransform(GLSLProgram &program);
	void updateProgramModelViewTransform(GLSLProgram &program);
	void setGLProjectionMatrix(const Mat4 &mat);
	void discardTemporaryData();
	void setProgram(GLSLProgram &program, bool updateModelViewTranform);
	GLuint makeProgram(GLuint vShader, GLuint fShader);
	bool linkProgram(GLuint program);
	void bindTempVertexBuffer();
	TextureRef newTex();
	void deleteTex(TextureRef texRef);
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	void glcMatrixMode(GLenum mode);
	#endif
	void glcBindTexture(GLenum target, GLuint texture);
	void glcDeleteTextures(GLsizei n, const GLuint *textures);
	void glcBlendFunc(GLenum sfactor, GLenum dfactor);
	void glcBlendEquation(GLenum mode);
	void glcEnable(GLenum cap);
	void glcDisable(GLenum cap);
	GLboolean glcIsEnabled(GLenum cap);
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	void glcEnableClientState(GLenum cap);
	void glcDisableClientState(GLenum cap);
	void glcTexEnvi(GLenum target, GLenum pname, GLint param);
	void glcTexEnvfv(GLenum target, GLenum pname, const GLfloat *params);
	void glcColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
	void glcTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	void glcColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	void glcVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	void glcVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
	#endif
	void glcBindBuffer(GLenum target, GLuint buffer);
	void glcDeleteBuffers(GLsizei n, const GLuint *buffers);
	void glcPixelStorei(GLenum pname, GLint param);
};

using RendererImpl = GLRenderer;

}
