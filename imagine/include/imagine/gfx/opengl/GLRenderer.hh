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
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/RendererTask.hh>
#include <imagine/gfx/BasicEffect.hh>
#include <imagine/gfx/Quads.hh>
#include <imagine/util/used.hh>
#include <memory>
#include <optional>
#include <string_view>
#ifdef CONFIG_BASE_GL_PLATFORM_EGL
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif

#ifndef GL_R8
#define GL_R8 0x8229
#endif

#ifndef GL_RG
#define GL_RG 0x8227
#endif

#ifndef GL_RG8
#define GL_RG8 0x822B
#endif

#ifndef GL_RED
#define GL_RED 0x1903
#endif

namespace IG
{
class ApplicationContext;
}

namespace IG::Gfx
{

class RendererCommands;
class TextureSampler;
class GLSLProgram;
class RendererTask;

class DrawContextSupport
{
public:
	#ifdef CONFIG_GFX_OPENGL_ES
	void (* GL_APIENTRY glGenSamplers) (GLsizei count, GLuint* samplers){};
	void (* GL_APIENTRY glDeleteSamplers) (GLsizei count, const GLuint* samplers){};
	void (* GL_APIENTRY glBindSampler) (GLuint unit, GLuint sampler){};
	void (* GL_APIENTRY glSamplerParameteri) (GLuint sampler, GLenum pname, GLint param){};
	void (* GL_APIENTRY glTexStorage2D) (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height){};
	void (* GL_APIENTRY glBindVertexArray) (GLuint array){};
	void (* GL_APIENTRY glDeleteVertexArrays) (GLsizei n, const GLuint *arrays){};
	void (* GL_APIENTRY glGenVertexArrays) (GLsizei n, GLuint *arrays){};
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
	static void glBindVertexArray(GLuint array) { ::glBindVertexArray(array); };
	static void glDeleteVertexArrays(GLsizei n, const GLuint *arrays) { ::glDeleteVertexArrays(n, arrays); };
	static void glGenVertexArrays(GLsizei n, GLuint *arrays) { ::glGenVertexArrays(n, arrays); };
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
	#endif
	using GLDebugMessageCallback = void (GL_APIENTRY *)(GLDEBUGPROC callback, const void *userParam);
	ConditionalMember<Config::OpenGLDebugContext, GLDebugMessageCallback> glDebugMessageCallback{};
	ConditionalMemberOr<(bool)Config::Gfx::OPENGL_ES, GLenum, GL_RED> luminanceFormat{GL_LUMINANCE};
	ConditionalMemberOr<(bool)Config::Gfx::OPENGL_ES, GLenum, GL_R8>  luminanceInternalFormat{GL_LUMINANCE8};
	ConditionalMemberOr<(bool)Config::Gfx::OPENGL_ES, GLenum, GL_RG>  luminanceAlphaFormat{GL_LUMINANCE_ALPHA};
	ConditionalMemberOr<(bool)Config::Gfx::OPENGL_ES, GLenum, GL_RG8> luminanceAlphaInternalFormat{GL_LUMINANCE8_ALPHA8};
	ConditionalMemberOr<(bool)Config::Gfx::OPENGL_ES, GLenum, GL_RED> alphaFormat{GL_ALPHA};
	ConditionalMemberOr<(bool)Config::Gfx::OPENGL_ES, GLenum, GL_R8>  alphaInternalFormat{GL_ALPHA8};
	TextureSizeSupport textureSizeSupport;
	//bool hasMemoryBarrier = false;
	bool hasImmutableTexStorage{};
	ConditionalMemberOr<(bool)Config::Gfx::OPENGL_ES, bool, true> hasBGRPixels{};
	ConditionalMemberOr<(bool)Config::Gfx::OPENGL_ES, bool, true> hasTextureSwizzle{};
	ConditionalMemberOr<(bool)Config::Gfx::OPENGL_ES, bool, true> hasUnpackRowLength{};
	ConditionalMemberOr<(bool)Config::Gfx::OPENGL_ES, bool, true> hasSamplerObjects{};
	ConditionalMemberOr<(bool)Config::Gfx::OPENGL_ES, bool, true> hasPBOFuncs{};
	ConditionalMemberOr<(bool)Config::Gfx::OPENGL_ES, bool, false> useLegacyGLSL{true};
	ConditionalMemberOr<(bool)Config::Gfx::OPENGL_ES, bool, true> hasSrgbWriteControl{};
	ConditionalMember<Config::OpenGLDebugContext, bool> hasDebugOutput{};
	ConditionalMember<!Config::Gfx::OPENGL_ES, bool> hasBufferStorage{};
	ConditionalMember<Config::envIsAndroid, bool> hasEGLImages{};
	ConditionalMember<Config::Gfx::OPENGL_TEXTURE_TARGET_EXTERNAL, bool> hasExternalEGLImages{};
	bool isConfigured{};

	bool hasDrawReadBuffers() const;
	bool hasSyncFences() const;
	bool hasServerWaitSync() const;
	bool hasEGLTextureStorage() const;
	bool hasImmutableBufferStorage() const;
	bool hasMemoryBarriers() const;
	bool hasVAOFuncs() const;
	GLsync fenceSync(GLDisplay dpy);
	void deleteSync(GLDisplay dpy, GLsync sync);
	GLenum clientWaitSync(GLDisplay dpy, GLsync sync, GLbitfield flags, GLuint64 timeout);
	void waitSync(GLDisplay dpy, GLsync sync);
	void setGLDebugOutput(bool on);
};

class GLRenderer
{
public:
	DrawContextSupport support{};
	[[no_unique_address]] GLManager glManager;
	RendererTask mainTask;
	BasicEffect basicEffect_{};
	Gfx::QuadIndexArray<uint8_t> quadIndices;
	CustomEvent releaseShaderCompilerEvent;

	GLRenderer(ApplicationContext);
	GLDisplay glDisplay() const;
	bool makeWindowDrawable(RendererTask &task, Window &, GLBufferConfig, GLColorSpace);
	int toSwapInterval(const Window &win, PresentMode mode) const;

protected:
	void addEventHandlers(ApplicationContext, RendererTask &);
	std::optional<GLBufferConfig> makeGLBufferConfig(ApplicationContext, PixelFormat, const Window * = {});
	void setCurrentDrawable(GLDisplay, GLContext, Drawable);
	void setupNonPow2MipmapRepeatTextures();
	void setupImmutableTexStorage(bool extSuffix);
	void setupRGFormats();
	void setupSamplerObjects();
	void setupSpecifyDrawReadBuffers();
	void setupUnmapBufferFunc();
	void setupImmutableBufferStorage();
	void setupMemoryBarrier();
	void setupVAOFuncs(bool oes = false);
	void setupFenceSync();
	void setupAppleFenceSync();
	void setupEglFenceSync(std::string_view eglExtenstionStr);
	void checkExtensionString(std::string_view extStr);
	bool attachWindow(Window &, GLBufferConfig, GLColorSpace);
	NativeWindowFormat nativeWindowFormat(GLBufferConfig) const;
	bool initBasicEffect();
};

using RendererImpl = GLRenderer;

}
