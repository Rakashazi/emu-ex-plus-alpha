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
#include <imagine/base/MessagePort.hh>
#include <imagine/gfx/defs.hh>
#include <imagine/gfx/TextureSizeSupport.hh>
#include <imagine/util/Interpolator.hh>
#include <imagine/util/typeTraits.hh>
#include <memory>
#include <thread>

namespace IG
{
class Semaphore;
}

namespace Gfx
{

#ifndef __APPLE__
#define CONFIG_GFX_OPENGL_DEBUG_CONTEXT
#endif

class RendererCommands;
class TextureSampler;
class GLSLProgram;

class DrawContextSupport
{
public:
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
	GLsync (* GL_APIENTRY glFenceSync) (GLenum condition, GLbitfield flags){};
	void (* GL_APIENTRY glDeleteSync) (GLsync sync){};
	GLenum (* GL_APIENTRY glClientWaitSync) (GLsync sync, GLbitfield flags, GLuint64 timeout){};
	void (* GL_APIENTRY glWaitSync) (GLsync sync, GLbitfield flags, GLuint64 timeout){};
	void (* GL_APIENTRY glBufferStorage) (GLenum target, GLsizeiptr size, const void *data, GLbitfield flags){};
	void (* GL_APIENTRY glFlushMappedBufferRange)(GLenum target, GLintptr offset, GLsizeiptr length){};
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
	static GLsync glFenceSync(GLenum condition, GLbitfield flags) { return ::glFenceSync(condition, flags); }
	static void glDeleteSync(GLsync sync) { ::glDeleteSync(sync); }
	static GLenum glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) { return ::glClientWaitSync(sync, flags, timeout); }
	static void glWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) { ::glWaitSync(sync, flags, timeout); }
	static void glBufferStorage(GLenum target, GLsizeiptr size, const void *data, GLbitfield flags) { ::glBufferStorage(target, size, data, flags); }
	static void glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length) { ::glFlushMappedBufferRange(target, offset, length); }
	#endif
	#ifdef __ANDROID__
	void (GL_APIENTRYP glEGLImageTargetTexStorageEXT)(GLenum target, GLeglImageOES image, const GLint* attrib_list){};
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
	GLenum bgrInternalFormat = GL_BGRA;
	TextureSizeSupport textureSizeSupport{};
	#ifndef CONFIG_GFX_OPENGL_ES
	GLfloat maximumAnisotropy{};
	bool hasMultisample = false;
	bool hasMultisampleHints = false;
	bool hasFenceSync = false;
	bool hasBufferStorage = false;
	#endif
	bool hasBGRPixels = false;
	bool hasVBOFuncs = false;
	bool hasTextureSwizzle = false;
	bool hasUnpackRowLength = !Config::Gfx::OPENGL_ES;
	bool hasSamplerObjects = !Config::Gfx::OPENGL_ES;
	bool hasImmutableTexStorage = false;
	bool hasPBOFuncs = false;
	#ifdef CONFIG_GFX_OPENGL_DEBUG_CONTEXT
	bool hasDebugOutput = false;
	#else
	static constexpr bool hasDebugOutput = false;
	#endif
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
	bool isConfigured = false;

	bool hasDrawReadBuffers() const;
	bool hasSyncFences() const;
	bool hasEGLTextureStorage() const;
	bool hasImmutableBufferStorage() const;
};

class GLMainTask
{
public:
	class TaskContext
	{
	public:
		constexpr TaskContext(IG::Semaphore *semPtr, bool *notifySemaporeAfterDelegatePtr):
			semPtr{semPtr}, notifySemaporeAfterDelegatePtr{notifySemaporeAfterDelegatePtr}
		{}
		void notifySemaphore();

	protected:
		IG::Semaphore *semPtr{};
		bool *notifySemaporeAfterDelegatePtr{};
	};

	using FuncDelegate = DelegateFunc2<sizeof(uintptr_t)*4 + sizeof(int)*10, void(TaskContext)>;

	enum class Command: uint8_t
	{
		UNSET, RUN_FUNC, EXIT
	};

	struct CommandMessage
	{
		IG::Semaphore *semPtr{};
		union Args
		{
			struct RunArgs
			{
				FuncDelegate func;
			} run;
		} args{};
		Command command{Command::UNSET};

		constexpr CommandMessage() {}
		constexpr CommandMessage(Command command, IG::Semaphore *semPtr = nullptr):
			semPtr{semPtr}, command{command} {}
		constexpr CommandMessage(Command command, FuncDelegate funcDel, IG::Semaphore *semPtr = nullptr):
			semPtr{semPtr}, args{funcDel}, command{command} {}
		explicit operator bool() const { return command != Command::UNSET; }
		void setReplySemaphore(IG::Semaphore *semPtr_) { assert(!semPtr); semPtr = semPtr_; };
	};

	~GLMainTask();
	void start(Base::GLContext context);
	void runFunc(FuncDelegate del, bool awaitReply = false);
	void stop();
	bool isStarted() const;
	void runPendingTasksOnThisThread();

private:
	Base::MessagePort<CommandMessage> commandPort{"GLMainTask Command"};
	std::thread thread{};
	bool started = false;
};

class GLRenderer
{
public:
	DrawContextSupport support{};
	std::unique_ptr<GLMainTask> mainTask{};
	Base::GLDisplay glDpy{};
	Base::GLContext gfxResourceContext{};
	Base::GLBufferConfig gfxBufferConfig{};
	TextureSampler defaultClampSampler{};
	TextureSampler defaultNearestMipClampSampler{};
	TextureSampler defaultNoMipClampSampler{};
	TextureSampler defaultNoLinearNoMipClampSampler{};
	TextureSampler defaultRepeatSampler{};
	TextureSampler defaultNearestMipRepeatSampler{};
	Base::CustomEvent releaseShaderCompilerEvent{"GLRenderer::releaseShaderCompilerEvent"};
	Base::ExitDelegate onExit{};
	IG::Semaphore resourceFenceSem{0};
	SyncFence resourceFence{};
	TimedInterpolator<Gfx::GC> projAngleM;
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	GLuint defaultVShader = 0;
	#endif
	Angle projectionMatRot = 0;
	GLuint samplerNames = 0; // used when separate sampler objects not supported
	bool resourceUpdate = false;
	bool resourceFenceQueued = false;
	bool useSeparateDrawContext = false;
	#ifndef CONFIG_GFX_OPENGL_ES
	bool useStreamVAO = false;
	#endif
	#if CONFIG_GFX_OPENGL_ES_MAJOR_VERSION > 1
	uint8_t glMajorVer = 0;
	#endif
	[[no_unique_address]] IG::UseTypeIf<Config::DEBUG_BUILD, bool> contextDestroyed = false;
	[[no_unique_address]] IG::UseTypeIf<Config::DEBUG_BUILD, bool> drawContextDebug = false;

	GLRenderer() {}
	void addEventHandlers();
	Base::GLContextAttributes makeKnownGLContextAttributes();
	void finishContextCreation(Base::GLContext ctx);
	void setCurrentDrawable(Base::GLDisplay dpy, Base::GLContext ctx, Drawable win);
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
	void setupImmutableBufferStorage();
	void setupFenceSync();
	void setupAppleFenceSync();
	void setupEGLFenceSync(bool supportsServerSync);
	void checkExtensionString(const char *extStr, bool &useFBOFuncs);
	void checkFullExtensionString(const char *fullExtStr);
	void verifyCurrentResourceContext();
	void setGLProjectionMatrix(RendererCommands &cmds, const Mat4 &mat);
	void setProgram(GLSLProgram &program);
	GLuint makeProgram(GLuint vShader, GLuint fShader);
	bool linkProgram(GLuint program);
	TextureSampler &commonTextureSampler(CommonTextureSampler sampler);
	bool hasGLTask() const;
	void runGLTask2(GLMainTask::FuncDelegate del, bool awaitReply);
	template<class Func>
	void runGLTask(Func &&del, bool awaitReply = false) { runGLTask2(wrapGLMainTaskDelegate(del), awaitReply); }
	template<class Func>
	void runGLTaskSync(Func &&del) { runGLTask2(wrapGLMainTaskDelegate(del), true); }

	template<class Func = GLMainTask::FuncDelegate>
	static GLMainTask::FuncDelegate wrapGLMainTaskDelegate(Func del)
	{
		constexpr auto args = IG::functionTraitsArity<Func>;
		if constexpr(args == 0)
		{
			// for void ()
			return
				[=](GLMainTask::TaskContext)
				{
					del();
				};
		}
		else
		{
			// for void (GLMainTask::TaskContext)
			return del;
		}
	}

	// for iOS EAGLView renderbuffer management
	IG::Point2D<int> makeIOSDrawableRenderbuffer(void *layer, GLuint &colorRenderbuffer, GLuint &depthRenderbuffer);
	void deleteIOSDrawableRenderbuffer(GLuint colorRenderbuffer, GLuint depthRenderbuffer);
	void setIOSDrawableDelegates();
};

using RendererImpl = GLRenderer;

}
