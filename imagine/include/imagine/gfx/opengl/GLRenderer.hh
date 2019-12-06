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
#include <imagine/base/MessagePort.hh>
#include <imagine/thread/Semaphore.hh>
#include <imagine/gfx/defs.hh>
#include <imagine/gfx/TextureSizeSupport.hh>
#include <imagine/gfx/Texture.hh>
#include <imagine/gfx/opengl/GLStateCache.hh>
#include <imagine/util/Interpolator.hh>
#include <memory>
#include <mutex>
#include <condition_variable>

namespace Gfx
{

#ifndef __APPLE__
#define CONFIG_GFX_OPENGL_DEBUG_CONTEXT
#endif

class RendererTask;
class RenderTarget;
class SyncFence;

class GLSyncFence
{
public:
	static constexpr uint64_t IGNORE_TIMEOUT = 0xFFFFFFFFFFFFFFFFull;
	GLsync sync{};

	constexpr GLSyncFence() {}
	constexpr GLSyncFence(GLsync sync): sync{sync} {}
};

using SyncFenceImpl = GLSyncFence;

class GLDrawableHolder
{
public:
	void makeDrawable(Renderer &r, Base::Window &win);
	void destroyDrawable(Renderer &r);
	Drawable drawable() const { return drawable_; };

protected:
	Drawable drawable_;
	Base::ResumeDelegate onResume;
	Base::ExitDelegate onExit;
};

using DrawableHolderImpl = GLDrawableHolder;

class GLRendererTask
{
public:
	enum class Command: uint8_t
	{
		UNSET, DRAW, RUN_FUNC, EXIT
	};

	enum class Reply: uint8_t
	{
		UNSET, DRAW_FINISHED
	};

	struct CommandMessage
	{
		IG::Semaphore *semAddr{};
		union Args
		{
			struct DrawArgs
			{
				DrawDelegate del;
				Base::Window *winPtr;
				Drawable drawable;
			} draw;
			struct RunFuncArgs
			{
				RenderTaskFuncDelegate func;
			} runFunc;
		} args{};
		Command command{Command::UNSET};

		constexpr CommandMessage() {}
		constexpr CommandMessage(Command command, IG::Semaphore *semAddr = nullptr):
			semAddr{semAddr}, command{command} {}
		constexpr CommandMessage(Command command, DrawDelegate drawDel, Drawable drawable, Base::Window &win, IG::Semaphore *semAddr = nullptr):
			semAddr{semAddr}, args{drawDel, &win, drawable}, command{command} {}
		constexpr CommandMessage(Command command, RenderTaskFuncDelegate func, IG::Semaphore *semAddr = nullptr):
			semAddr{semAddr}, command{command}
		{
			args.runFunc.func = func;
		}
		explicit operator bool() const { return command != Command::UNSET; }
	};

	struct ReplyMessage
	{
		Reply reply{Reply::UNSET};
		DrawFinishedParams params{};

		constexpr ReplyMessage() {}
		constexpr ReplyMessage(Reply reply, DrawFinishedParams params):
			reply{reply}, params{params} {}
		explicit operator bool() const { return reply != Reply::UNSET; }
	};

	Base::GLContext glContext() const { return glCtx; };
	void initVBOs();
	GLuint getVBO();
	void initVAO();
	void initDefaultFramebuffer();
	GLuint defaultFBO() const { return defaultFB; }
	GLuint bindFramebuffer(Texture &tex);
	void destroyContext(bool useSeparateDrawContext, Base::GLDisplay dpy);
	bool handleDrawableReset();

protected:
	Base::MessagePort<CommandMessage> commandPort{};
	Base::MessagePort<ReplyMessage> replyPort{};
	Base::GLContext glCtx{};
	DelegateFuncSet<DrawFinishedDelegate> onDrawFinished{};
	Base::ResumeDelegate onResume{};
	Base::ExitDelegate onExit{};
	Base::FrameTimeBase drawTimestamp{};
	std::mutex drawMutex{};
	std::condition_variable drawCondition{};
	#ifndef CONFIG_GFX_OPENGL_ES
	GLuint streamVAO = 0;
	std::array<GLuint, 6> streamVBO{};
	uint streamVBOIdx = 0;
	#endif
	#ifdef CONFIG_GLDRAWABLE_NEEDS_FRAMEBUFFER
	GLuint defaultFB = 0;
	#else
	static constexpr GLuint defaultFB = 0;
	#endif
	GLuint fbo = 0;
	bool resetDrawable = false;
	bool canDraw = true;

	void replyHandler(Renderer &r, ReplyMessage msg);
	bool commandHandler(decltype(commandPort)::Messages messages, Base::GLDisplay glDpy, bool ownsThread);
};

using RendererTaskImpl = GLRendererTask;

class GLRendererDrawTask
{
public:
	GLRendererDrawTask(RendererTask &task, Base::GLDisplay glDpy);
	void setCurrentDrawable(Drawable win);
	void present(Drawable win);
	GLuint bindFramebuffer(Texture &t);
	GLuint getVBO();
	GLuint defaultFramebuffer() const;
	Base::GLDisplay glDisplay() const { return glDpy; };

protected:
	RendererTask &task;
	Base::GLDisplay glDpy{};
};

using RendererDrawTaskImpl = GLRendererDrawTask;

class DrawContextSupport
{
public:
	bool isConfigured = false;
	#ifndef CONFIG_GFX_OPENGL_ES
	bool hasAnisotropicFiltering = false;
	bool hasMultisample = false;
	bool hasMultisampleHints = false;
	bool hasFenceSync = false;
	#endif
	bool hasBGRPixels = false;
	GLenum bgrInternalFormat = GL_BGRA;
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
	TextureSizeSupport textureSizeSupport{};

	bool hasDrawReadBuffers() const;
	bool hasSyncFences() const;
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
	#endif
	GLenum luminanceFormat = GL_LUMINANCE;
	GLenum luminanceInternalFormat = GL_LUMINANCE8;
	GLenum luminanceAlphaFormat = GL_LUMINANCE_ALPHA;
	GLenum luminanceAlphaInternalFormat = GL_LUMINANCE8_ALPHA8;
	GLenum alphaFormat = GL_ALPHA;
	GLenum alphaInternalFormat = GL_ALPHA8;
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
};

class GLMainTask
{
public:
	struct TaskContext {};

	using FuncDelegate = DelegateFunc2<sizeof(uintptr_t)*4 + sizeof(int)*10, void(TaskContext)>;

	enum class Command: uint8_t
	{
		UNSET, RUN_FUNC, EXIT
	};

	struct CommandMessage
	{
		IG::Semaphore *semAddr{};
		union Args
		{
			struct RunArgs
			{
				FuncDelegate func;
			} run;
		} args{};
		Command command{Command::UNSET};

		constexpr CommandMessage() {}
		constexpr CommandMessage(Command command, IG::Semaphore *semAddr = nullptr):
			semAddr{semAddr}, command{command} {}
		constexpr CommandMessage(Command command, FuncDelegate funcDel, IG::Semaphore *semAddr):
			semAddr{semAddr}, args{funcDel}, command{command} {}
		explicit operator bool() const { return command != Command::UNSET; }
	};

	~GLMainTask();
	void start(Base::GLContext context);
	void runFunc(FuncDelegate del, IG::Semaphore *semAddr = nullptr);
	void runFuncSync(FuncDelegate del);
	void stop();

private:
	Base::MessagePort<CommandMessage> commandPort{};
	bool started = false;
};

class GLRenderer
{
public:
	Base::GLDisplay glDpy{};
	Base::GLContext gfxResourceContext{};
	Base::GLBufferConfig gfxBufferConfig{};
	bool resourceUpdate = false;
	bool useSeparateDrawContext = false;
	#ifndef CONFIG_GFX_OPENGL_ES
	bool useStreamVAO = false;
	#endif
	#if CONFIG_GFX_OPENGL_ES_MAJOR_VERSION > 1
	uint8_t glMajorVer = 0;
	#endif
	#ifndef NDEBUG
	bool contextDestroyed = false;
	bool drawContextDebug = false;
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	GLuint defaultVShader = 0;
	#endif
	Angle projectionMatRot = 0;
	GLuint samplerNames = 0; // used when separate sampler objects not supported
	TextureSampler defaultClampSampler{};
	TextureSampler defaultNearestMipClampSampler{};
	TextureSampler defaultNoMipClampSampler{};
	TextureSampler defaultNoLinearNoMipClampSampler{};
	TextureSampler defaultRepeatSampler{};
	TextureSampler defaultNearestMipRepeatSampler{};
	Base::Timer releaseShaderCompilerTimer;
	Base::ExitDelegate onExit{};
	TimedInterpolator<Gfx::GC> projAngleM;
	std::unique_ptr<GLMainTask> mainTask{};
	DrawContextSupport support{};

	GLRenderer() {}
	void addOnExitHandler();
	Base::GLContextAttributes makeKnownGLContextAttributes();
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
	void setupFenceSync();
	void setupAppleFenceSync();
	void setupEGLFenceSync(bool supportsServerSync);
	void checkExtensionString(const char *extStr, bool &useFBOFuncs);
	void checkFullExtensionString(const char *fullExtStr);
	void verifyCurrentResourceContext();
	void verifyCurrentTexture2D(TextureRef tex);
	void setGLProjectionMatrix(RendererCommands &cmds, const Mat4 &mat);
	void setProgram(GLSLProgram &program);
	GLuint makeProgram(GLuint vShader, GLuint fShader);
	bool linkProgram(GLuint program);
	void runGLTask2(GLMainTask::FuncDelegate del, IG::Semaphore *semAddr = nullptr);
	template<class FUNC>
	void runGLTask(FUNC &&del, IG::Semaphore *semAddr = nullptr) { runGLTask2(wrapGLMainTaskDelegate(del), semAddr); }
	void runGLTaskSync2(GLMainTask::FuncDelegate del);
	template<class FUNC>
	void runGLTaskSync(FUNC &&del) { runGLTaskSync2(wrapGLMainTaskDelegate(del)); }
	template<class FUNC>
	void runGLTaskSyncConditional(FUNC &&del, bool shouldRunSync, IG::Semaphore *semAddr = nullptr)
	{
		if(shouldRunSync)
			runGLTaskSync2(wrapGLMainTaskDelegate(del));
		else
			runGLTask2(wrapGLMainTaskDelegate(del), semAddr);
	}

	template<class FUNC = GLMainTask::FuncDelegate>
	static GLMainTask::FuncDelegate wrapGLMainTaskDelegate(FUNC del)
	{
		constexpr auto args = IG::functionTraitsArity<FUNC>;
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

class GLRendererCommands
{
public:
	TextureSampler currSampler{};
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	uint currentVtxArrayPointerID = 0;
	Mat4 modelMat, projectionMat;
	#endif
	GLStateCache glState{};

	void discardTemporaryData();
	void bindGLArrayBuffer(GLuint vbo);
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

protected:
	Viewport currViewport;
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	GLSLProgram *currProgram{};
	#endif
	std::array<ColorComp, 4> vColor{}; // color when using shader pipeline
	std::array<ColorComp, 4> texEnvColor{}; // color when using shader pipeline
	GLuint arrayBuffer = 0;
	bool arrayBufferIsSet = false;
};

using RendererCommandsImpl = GLRendererCommands;

}
