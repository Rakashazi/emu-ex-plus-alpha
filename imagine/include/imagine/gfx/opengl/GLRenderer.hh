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
#include <imagine/base/CustomEvent.hh>
#include <imagine/gfx/defs.hh>
#include <imagine/gfx/TextureSizeSupport.hh>
#include <imagine/gfx/TextureSampler.hh>
#include <imagine/gfx/Program.hh>
#include <imagine/gfx/RendererTask.hh>
#include <imagine/util/typeTraits.hh>
#include <memory>
#ifdef CONFIG_BASE_GL_PLATFORM_EGL
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif

namespace IG
{
class Semaphore;
}

namespace Base
{
class ApplicationContext;
}

namespace Gfx
{

class RendererCommands;
class TextureSampler;
class GLSLProgram;
class RendererTask;

class DrawContextSupport
{
public:
	#ifdef CONFIG_MACHINE_PANDORA
	using EGLSync = void*;
	using EGLTime = uint64_t;
	#endif
	#ifdef CONFIG_GFX_OPENGL_ES
	void (* GL_APIENTRY glGenSamplers) (GLsizei count, GLuint* samplers){};
	void (* GL_APIENTRY glDeleteSamplers) (GLsizei count, const GLuint* samplers){};
	void (* GL_APIENTRY glBindSampler) (GLuint unit, GLuint sampler){};
	void (* GL_APIENTRY glSamplerParameteri) (GLuint sampler, GLenum pname, GLint param){};
	void (* GL_APIENTRY glTexStorage2D) (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height){};
	GLvoid* (* GL_APIENTRY glMapBufferRange) (GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access){};
	using UnmapBufferProto = GLboolean (* GL_APIENTRY) (GLenum target);
	UnmapBufferProto glUnmapBuffer{};
	//void (* GL_APIENTRY glDrawBuffers) (GLsizei size, const GLenum *bufs){};
	//void (* GL_APIENTRY glReadBuffer) (GLenum src){};
	void (* GL_APIENTRY glBufferStorage) (GLenum target, GLsizeiptr size, const void *data, GLbitfield flags){};
	void (* GL_APIENTRY glFlushMappedBufferRange)(GLenum target, GLintptr offset, GLsizeiptr length){};
	//void (* GL_APIENTRY glMemoryBarrier) (GLbitfield barriers){};
		#ifdef CONFIG_BASE_GL_PLATFORM_EGL
		// Prototypes based on EGL_KHR_fence_sync/EGL_KHR_wait_sync versions
		EGLSync (EGLAPIENTRY *eglCreateSync)(EGLDisplay dpy, EGLenum type, const EGLint *attrib_list){};
		EGLBoolean (EGLAPIENTRY *eglDestroySync)(EGLDisplay dpy, EGLSync sync){};
		EGLint (EGLAPIENTRY *eglClientWaitSync)(EGLDisplay dpy, EGLSync sync, EGLint flags, EGLTime timeout){};
		//EGLint (EGLAPIENTRY *eglWaitSync)(EGLDisplay dpy, EGLSync sync, EGLint flags){};
		#else
		GLsync (* GL_APIENTRY glFenceSync) (GLenum condition, GLbitfield flags){};
		void (* GL_APIENTRY glDeleteSync) (GLsync sync){};
		GLenum (* GL_APIENTRY glClientWaitSync) (GLsync sync, GLbitfield flags, GLuint64 timeout){};
		//void (* GL_APIENTRY glWaitSync) (GLsync sync, GLbitfield flags, GLuint64 timeout){};
		#endif
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
	static void glBufferStorage(GLenum target, GLsizeiptr size, const void *data, GLbitfield flags) { ::glBufferStorage(target, size, data, flags); }
	static void glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length) { ::glFlushMappedBufferRange(target, offset, length); }
	//static void glMemoryBarrier(GLbitfield barriers) { ::glMemoryBarrier(barriers); }
		#ifdef CONFIG_BASE_GL_PLATFORM_EGL
		static EGLSync eglCreateSync(EGLDisplay dpy, EGLenum type, const EGLAttrib *attrib_list) { return ::eglCreateSync(dpy, type, attrib_list); }
		static EGLBoolean eglDestroySync(EGLDisplay dpy, EGLSync sync) { return ::eglDestroySync(dpy, sync); }
		static EGLint eglClientWaitSync(EGLDisplay dpy, EGLSync sync, EGLint flags, EGLTime timeout) { return ::eglClientWaitSync(dpy, sync, flags, timeout); }
		//static EGLBoolean eglWaitSync(EGLDisplay dpy, EGLSync sync, EGLint flags) { return ::eglWaitSync(dpy, sync, flags); }
		#else
		static GLsync glFenceSync(GLenum condition, GLbitfield flags) { return ::glFenceSync(condition, flags); }
		static void glDeleteSync(GLsync sync) { ::glDeleteSync(sync); }
		static GLenum glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) { return ::glClientWaitSync(sync, flags, timeout); }
		//static void glWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) { ::glWaitSync(sync, flags, timeout); }
		#endif
	#endif
	#ifdef __ANDROID__
	void (GL_APIENTRYP glEGLImageTargetTexStorageEXT)(GLenum target, GLeglImageOES image, const GLint* attrib_list){};
	EGLBoolean (EGLAPIENTRY *eglPresentationTimeANDROID)(EGLDisplay dpy, EGLSurface surface, EGLnsecsANDROID time){};
	#endif
	#if defined CONFIG_GFX_OPENGL_DEBUG_CONTEXT && defined CONFIG_GFX_OPENGL_ES
	void GL_APIENTRY (*glDebugMessageCallback)(GLDEBUGPROCKHR callback, const void *userParam){};
	static constexpr auto DEBUG_OUTPUT = GL_DEBUG_OUTPUT_KHR;
	#elif defined CONFIG_GFX_OPENGL_DEBUG_CONTEXT
	void GL_APIENTRY (*glDebugMessageCallback)(GLDEBUGPROC callback, const void *userParam){};
	static constexpr auto DEBUG_OUTPUT = GL_DEBUG_OUTPUT;
	#endif
	#if defined CONFIG_GFX_OPENGL_ES && CONFIG_GFX_OPENGL_ES_MAJOR_VERSION > 1
	static void generateMipmaps(GLenum target) { ::glGenerateMipmap(target); };
	#else
	using GenerateMipmapsProto = void (* GL_APIENTRY)(GLenum target);
	GenerateMipmapsProto generateMipmaps{}; // set via extensions
	#endif
	GLenum luminanceFormat = GL_LUMINANCE;
	GLenum luminanceInternalFormat = GL_LUMINANCE8;
	GLenum luminanceAlphaFormat = GL_LUMINANCE_ALPHA;
	GLenum luminanceAlphaInternalFormat = GL_LUMINANCE8_ALPHA8;
	GLenum alphaFormat = GL_ALPHA;
	GLenum alphaInternalFormat = GL_ALPHA8;
	TextureSizeSupport textureSizeSupport{};
	//bool hasMemoryBarrier = false;
	IG_enableMemberIfOrConstant((bool)Config::Gfx::OPENGL_ES, bool, true, hasBGRPixels){};
	bool hasVBOFuncs{};
	bool hasTextureSwizzle{};
	bool hasUnpackRowLength = !Config::Gfx::OPENGL_ES;
	bool hasSamplerObjects = !Config::Gfx::OPENGL_ES;
	bool hasImmutableTexStorage{};
	bool hasPBOFuncs{};
	bool useLegacyGLSL = Config::Gfx::OPENGL_ES;
	IG_enableMemberIf(Config::Gfx::OPENGL_DEBUG_CONTEXT, bool, hasDebugOutput){};
	IG_enableMemberIf(!Config::Gfx::OPENGL_ES, bool, hasBufferStorage){};
	IG_enableMemberIf(Config::envIsAndroid, bool, hasEGLImages){};
	IG_enableMemberIf(Config::Gfx::OPENGL_TEXTURE_TARGET_EXTERNAL, bool, hasExternalEGLImages){};
	IG_enableMemberIf(Config::Gfx::OPENGL_FIXED_FUNCTION_PIPELINE, bool, useFixedFunctionPipeline){true};
	bool hasSrgbWriteControl{};
	bool isConfigured{};

	bool hasDrawReadBuffers() const;
	bool hasSyncFences() const;
	bool hasServerWaitSync() const;
	bool hasEGLTextureStorage() const;
	bool hasImmutableBufferStorage() const;
	bool hasMemoryBarriers() const;
	GLsync fenceSync(Base::GLDisplay dpy);
	void deleteSync(Base::GLDisplay dpy, GLsync sync);
	GLenum clientWaitSync(Base::GLDisplay dpy, GLsync sync, GLbitfield flags, GLuint64 timeout);
	void waitSync(Base::GLDisplay dpy, GLsync sync);
	void setGLDebugOutput(bool on);
};

struct GLCommonPrograms
{
	// color replacement
	Program texReplace{};
	Program texAlphaReplace{};
	// color modulation
	Program tex{};
	Program texAlpha{};
	// no texture
	Program noTex{};
	// external textures
	IG_enableMemberIf(Config::Gfx::OPENGL_TEXTURE_TARGET_EXTERNAL, Program, texExternalReplace){};
	IG_enableMemberIf(Config::Gfx::OPENGL_TEXTURE_TARGET_EXTERNAL, Program, texExternal){};
};

struct GLCommonSamplers
{
	TextureSampler clamp{};
	TextureSampler nearestMipClamp{};
	TextureSampler noMipClamp{};
	TextureSampler noLinearNoMipClamp{};
	TextureSampler repeat{};
	TextureSampler nearestMipRepeat{};
};

class GLRenderer
{
public:
	DrawContextSupport support{};
	[[no_unique_address]] Base::GLManager glManager;
	RendererTask mainTask;
	GLCommonPrograms commonProgram{};
	GLCommonSamplers commonSampler{};
	Base::CustomEvent releaseShaderCompilerEvent{Base::CustomEvent::NullInit{}};
	IG_enableMemberIf(Config::Gfx::OPENGL_SHADER_PIPELINE, GLuint, defaultVShader){};

	GLRenderer(Base::ApplicationContext, Error &err);
	void setGLProjectionMatrix(RendererCommands &cmds, Mat4 mat) const;
	void useCommonProgram(RendererCommands &cmds, CommonProgram program, const Mat4 *modelMat) const;
	Base::GLDisplay glDisplay() const;
	bool makeWindowDrawable(RendererTask &task, Base::Window &, Base::GLBufferConfig, Base::GLColorSpace);

protected:
	void addEventHandlers(Base::ApplicationContext, RendererTask &);
	std::optional<Base::GLBufferConfig> makeGLBufferConfig(Base::ApplicationContext, IG::PixelFormat, const Base::Window * = {});
	void setCurrentDrawable(Base::GLDisplay, Base::GLContext, Drawable);
	void setupNonPow2Textures();
	void setupNonPow2MipmapTextures();
	void setupNonPow2MipmapRepeatTextures();
	void setupBGRPixelSupport();
	void setupFBOFuncs(bool &useFBOFuncs);
	void setupTextureSwizzle();
	void setupImmutableTexStorage(bool extSuffix);
	void setupRGFormats();
	void setupSamplerObjects();
	void setupPBO();
	void setupSpecifyDrawReadBuffers();
	void setupUnmapBufferFunc();
	void setupImmutableBufferStorage();
	void setupMemoryBarrier();
	void setupFenceSync();
	void setupAppleFenceSync();
	void setupEglFenceSync(const char *eglExtenstionStr);
	void setupPresentationTime(const char *eglExtenstionStr);
	void checkExtensionString(const char *extStr, bool &useFBOFuncs);
	void checkFullExtensionString(const char *fullExtStr);
	const Program &commonProgramRef(CommonProgram program) const;
	bool attachWindow(Base::Window &, Base::GLBufferConfig, Base::GLColorSpace);
	Base::NativeWindowFormat nativeWindowFormat(Base::GLBufferConfig) const;
};

using RendererImpl = GLRenderer;

}
