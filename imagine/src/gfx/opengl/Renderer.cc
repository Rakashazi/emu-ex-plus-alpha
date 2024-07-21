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

#define LOGTAG "GLRenderer"
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererTask.hh>
#include <imagine/gfx/Texture.hh>
#include <imagine/gfx/PixmapBufferTexture.hh>
#include <imagine/gfx/TextureSampler.hh>
#include <imagine/logger/logger.h>
#include <imagine/base/Window.hh>
#include <imagine/base/GLContext.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Viewport.hh>
#include <imagine/data-type/image/PixmapSource.hh>
#include <imagine/util/opengl/glUtils.hh>
#include "internalDefs.hh"

namespace IG::Gfx
{

static_assert(!Config::Gfx::OPENGL_ES || Config::Gfx::OPENGL_ES >= 2);
static_assert((uint8_t)TextureBufferMode::DEFAULT == 0, "TextureBufferMode::DEFAULT != 0");

constexpr SystemLogger log{"GLRenderer"};
bool checkGLErrors = Config::DEBUG_BUILD;
bool checkGLErrorsVerbose = false;
[[gnu::weak]] const bool Renderer::enableSamplerObjects = false;

Renderer::Renderer(ApplicationContext ctx):
	GLRenderer{ctx} {}

Renderer::~Renderer()
{
	auto ctx = appContext();
	for(auto &w : ctx.windows())
	{
		detachWindow(*w);
	}
}

GLRenderer::GLRenderer(ApplicationContext ctx):
	glManager{ctx.nativeDisplayConnection(), glAPI},
	mainTask{ctx, "Main GL Context Messages", *static_cast<Renderer*>(this)},
	releaseShaderCompilerEvent
	{
		{.debugLabel = "GLRenderer::releaseShaderCompilerEvent"},
		[&task = mainTask, ctx]
		{
			if(!ctx.isRunning())
				return;
			logMsg("automatically releasing shader compiler");
			task.releaseShaderCompiler();
		}
	}
{
	if(!glManager)
	{
		throw std::runtime_error("Error getting GL display");
	}
	glManager.logInfo();
}

void Renderer::initMainTask(Window *initialWindow, DrawableConfig drawableConfig)
{
	if(mainTask.glContext())
	{
		return;
	}
	auto ctx = appContext();
	auto bufferConfig = makeGLBufferConfig(ctx, drawableConfig.pixelFormat, initialWindow);
	if(!bufferConfig) [[unlikely]]
	{
		throw std::runtime_error("Error finding a GL configuration");
	}
	Drawable initialDrawable{};
	if(initialWindow)
	{
		if(!GLRenderer::attachWindow(*initialWindow, *bufferConfig, (GLColorSpace)drawableConfig.colorSpace))
		{
			throw std::runtime_error("Error creating window surface");
		}
		initialDrawable = (Drawable)winData(*initialWindow).drawable;
	}
	GLTaskConfig conf
	{
		.glManagerPtr = &glManager,
		.bufferConfig = *bufferConfig,
		.initialDrawable = initialDrawable,
	};
	if(!mainTask.makeGLContext(conf)) [[unlikely]]
	{
		throw std::runtime_error("Error creating GL context");
	}
	addEventHandlers(ctx, mainTask);
	configureRenderer();
	if(!initBasicEffect()) [[unlikely]]
	{
		throw std::runtime_error("Error creating basic shader program");
	}
	quadIndices = {mainTask, 32};
}

NativeWindowFormat GLRenderer::nativeWindowFormat(GLBufferConfig bufferConfig) const
{
	return glManager.nativeWindowFormat(mainTask.appContext(), bufferConfig);
}

static float rotationRadians(Rotation r)
{
	switch(r)
	{
		case Rotation::ANY:
		case Rotation::UP: return radians(0.);
		case Rotation::RIGHT: return radians(90.);
		case Rotation::DOWN: return radians(-180.);
		case Rotation::LEFT: return radians(-90.);
	}
	bug_unreachable("Rotation == %d", std::to_underlying(r));
}

bool GLRenderer::attachWindow(Window &win, GLBufferConfig bufferConfig, GLColorSpace colorSpace)
{
	if(!win.hasSurface()) [[unlikely]]
	{
		logMsg("can't attach uninitialized window");
		return false;
	}
	logMsg("attaching window:%p", &win);
	auto &rData = win.makeRendererData<GLRendererWindowData>();
	if(!makeWindowDrawable(mainTask, win, bufferConfig, colorSpace)) [[unlikely]]
	{
		return false;
	}
	if(win.isMainWindow())
	{
		if(!Config::SYSTEM_ROTATES_WINDOWS)
		{
			rData.projAngleM = rotationRadians(win.softOrientation());
			win.appContext().setOnDeviceOrientationChanged(
				[this, &win](ApplicationContext, Rotation newO)
				{
					auto oldWinO = win.softOrientation();
					if(win.requestOrientationChange(newO))
					{
						static_cast<Renderer*>(this)->animateWindowRotation(win, rotationRadians(oldWinO), rotationRadians(newO));
					}
				});
		}
		else if(Config::SYSTEM_ROTATES_WINDOWS && !win.appContext().systemAnimatesWindowRotation())
		{
			win.appContext().setOnSystemOrientationChanged(
				[this, &win](ApplicationContext, Rotation oldO, Rotation newO)
				{
					const float orientationDiffTable[4][4]
					{
						{0, radians(90.), radians(-180.), radians(-90.)},
						{radians(-90.), 0, radians(90.), radians(-180.)},
						{radians(-180.), radians(-90.), 0, radians(90.)},
						{radians(90.), radians(-180.), radians(-90.), 0},
					};
					auto rotAngle = orientationDiffTable[std::to_underlying(oldO)][std::to_underlying(newO)];
					logMsg("animating from %d degrees", (int)degrees(rotAngle));
					static_cast<Renderer*>(this)->animateWindowRotation(win, rotAngle, 0.);
				});
		}
	}
	return true;
}

bool GLRenderer::makeWindowDrawable(RendererTask &task, Window &win, GLBufferConfig bufferConfig, GLColorSpace colorSpace)
{
	auto &rData = winData(win);
	rData.bufferConfig = bufferConfig;
	rData.colorSpace = colorSpace;
	rData.swapInterval = toSwapInterval(win, Gfx::PresentMode::Auto);
	task.destroyDrawable(rData.drawable);
	GLDrawableAttributes attr{bufferConfig};
	attr.colorSpace = colorSpace;
	try
	{
		rData.drawable = glManager.makeDrawable(win, attr);
	}
	catch(...)
	{
		return false;
	}
	return true;
}

bool Renderer::attachWindow(Window &win, DrawableConfig drawableConfig)
{
	GLBufferConfig bufferConfig = mainTask.glBufferConfig();
	if(canRenderToMultiplePixelFormats())
	{
		auto bufferConfigOpt = makeGLBufferConfig(appContext(), drawableConfig.pixelFormat, &win);
		if(!bufferConfigOpt) [[unlikely]]
		{
			return false;
		}
		bufferConfig = *bufferConfigOpt;
	}
	return GLRenderer::attachWindow(win, bufferConfig, (GLColorSpace)drawableConfig.colorSpace);
}

void Renderer::detachWindow(Window &win)
{
	win.resetRendererData();
	if(win.isMainWindow())
	{
		if(!Config::SYSTEM_ROTATES_WINDOWS)
		{
			win.appContext().setOnDeviceOrientationChanged({});
		}
		else if(Config::SYSTEM_ROTATES_WINDOWS && !win.appContext().systemAnimatesWindowRotation())
		{
			win.appContext().setOnSystemOrientationChanged({});
		}
	}
}

bool Renderer::setDrawableConfig(Window &win, DrawableConfig config)
{
	auto bufferConfig = makeGLBufferConfig(appContext(), config.pixelFormat, &win);
	if(!bufferConfig) [[unlikely]]
	{
		return false;
	}
	if(winData(win).bufferConfig == *bufferConfig && winData(win).colorSpace == (GLColorSpace)config.colorSpace)
	{
		return true;
	}
	if(mainTask.glBufferConfig() != *bufferConfig && !canRenderToMultiplePixelFormats())
	{
		// context only supports config it was created with
		return false;
	}
	win.setFormat(config.pixelFormat);
	return makeWindowDrawable(mainTask, win, *bufferConfig, (GLColorSpace)config.colorSpace);
}

void Renderer::setDefaultViewport(Window &win, Viewport v)
{
	winData(win).viewportRect = v.asYUpRelRect();
}

bool Renderer::canRenderToMultiplePixelFormats() const
{
	return glManager.hasNoConfigContext();
}

void Renderer::releaseShaderCompiler()
{
	task().releaseShaderCompiler();
}

void Renderer::autoReleaseShaderCompiler()
{
	releaseShaderCompilerEvent.notify();
}

ClipRect Renderer::makeClipRect(const Window &win, IG::WindowRect rect)
{
	int x = rect.x;
	int y = rect.y;
	int w = rect.xSize();
	int h = rect.ySize();
	//logMsg("scissor before transform %d,%d size %d,%d", x, y, w, h);
	// translate from view to window coordinates
	if(!Config::SYSTEM_ROTATES_WINDOWS)
	{
		switch(win.softOrientation())
		{
			default:
				y = win.height() - (y + h);
				break;
			case Rotation::RIGHT:
				std::swap(x, y);
				std::swap(w, h);
				x = (win.realWidth() - x) - w;
				y = (win.realHeight() - y) - h;
				break;
			case Rotation::LEFT:
				std::swap(x, y);
				std::swap(w, h);
				break;
			case Rotation::DOWN:
				x = (win.realWidth() - x) - w;
		}
	}
	else
	{
		//x += win.viewport.rect.x;
		y = win.height() - (y + h /*+ win.viewport.rect.y*/);
	}
	return {{x, y}, {w, h}};
}

bool Renderer::supportsSyncFences() const
{
	return support.hasSyncFences();
}

bool Renderer::supportsPresentationTime() const { return glManager.hasPresentationTime(); }

int GLRenderer::toSwapInterval(const Window &win, PresentMode mode) const
{
	switch(mode)
	{
		case PresentMode::Auto: return toSwapInterval(win, static_cast<const Renderer*>(this)->evalPresentMode(win, mode));
		case PresentMode::Immediate: return 0;
		case PresentMode::FIFO: return 1;
	}
	std::unreachable();
}

PresentMode Renderer::evalPresentMode(const Window&, PresentMode mode) const
{
	if(mode == PresentMode::Auto)
		return PresentMode::FIFO;
	return mode;
}

int Renderer::maxSwapChainImages() const
{
	#ifdef __ANDROID__
	if(appContext().androidSDK() < 18)
		return 2;
	#endif
	if(Config::envIsLinux)
	{
		if(const char *maxFramesEnv = getenv("__GL_MaxFramesAllowed");
			maxFramesEnv && maxFramesEnv[0] == '1')
		{
			return 2;
		}
	}
	return 3; // assume triple-buffering by default
}

bool Renderer::supportsColorSpace() const
{
	return glManager.hasSrgbColorSpace();
}

bool Renderer::hasSrgbColorSpaceWriteControl() const
{
	return support.hasSrgbWriteControl;
}

ColorSpace Renderer::supportedColorSpace(PixelFormat fmt, ColorSpace wantedColorSpace)
{
	switch(wantedColorSpace)
	{
		case ColorSpace::LINEAR: return ColorSpace::LINEAR;
		case ColorSpace::SRGB:
			switch(fmt.id)
			{
				case PixelFmtRGBA8888:
				case PixelFmtBGRA8888:
					return ColorSpace::SRGB;
				default: return ColorSpace::LINEAR;
			}
	}
	return ColorSpace::LINEAR;
}

ApplicationContext Renderer::appContext() const
{
	return task().appContext();
}

GLRendererWindowData &winData(Window &win)
{
	assumeExpr(win.rendererData<GLRendererWindowData>());
	return *win.rendererData<GLRendererWindowData>();
}

const GLRendererWindowData &winData(const Window &win)
{
	assumeExpr(win.rendererData<GLRendererWindowData>());
	return *win.rendererData<GLRendererWindowData>();
}

GLDisplay GLRenderer::glDisplay() const
{
	return glManager.display();
}

std::vector<DrawableConfigDesc> Renderer::supportedDrawableConfigs() const
{
	std::vector<DrawableConfigDesc> formats{};
	formats.reserve(3);
	static constexpr DrawableConfigDesc testDescs[]
	{
		{
			.name = "RGBA8888",
			.config{ .pixelFormat = PixelFmtRGBA8888 }
		},
		{
			.name = "RGBA8888:sRGB",
			.config{ .pixelFormat = PixelFmtRGBA8888, .colorSpace = ColorSpace::SRGB }
		},
		{
			.name = "RGB565",
			.config{ .pixelFormat = PixelFmtRGB565 }
		},
	};
	for(auto desc : testDescs)
	{
		if(glManager.hasDrawableConfig({desc.config.pixelFormat}, (GLColorSpace)desc.config.colorSpace))
		{
			formats.emplace_back(desc);
		}
	}
	return formats;
}

bool Renderer::hasBgraFormat(TextureBufferMode mode) const
{
	if constexpr(Config::envIsAndroid)
	{
		if(mode == TextureBufferMode::ANDROID_HARDWARE_BUFFER || mode == TextureBufferMode::ANDROID_SURFACE_TEXTURE)
			return false;
	}
	if(!support.hasBGRPixels)
		return false;
	if(Config::Gfx::OPENGL_ES && support.hasImmutableTexStorage)
	{
		return false;
	}
	return true;
}

bool GLRenderer::initBasicEffect()
{
	auto &rTask = mainTask;

	std::string_view vertShaderSrc =
R"(in vec4 pos;
in vec4 color;
in vec2 texUV;
out vec4 colorOut;
out vec2 texUVOut;
uniform mat4 modelView;
uniform mat4 proj;
void main() {
	colorOut = color;
	texUVOut = texUV;
	gl_Position = proj * modelView * pos;
})";

	std::string_view fragShaderSrc =
R"(in mediump vec4 colorOut;
in lowp vec2 texUVOut;
uniform sampler2D tex;
uniform lowp int textureMode;
void main() {
	if(textureMode == 1)
	{
		FRAGCOLOR = colorOut * texture(tex, texUVOut);
	}
#ifdef alphaTexSwizzle
	else if(textureMode == 2)
	{
		FRAGCOLOR.rgb = colorOut.rgb;
		FRAGCOLOR.a = colorOut.a * texture(tex, texUVOut).a;
	}
#endif
	else
	{
		FRAGCOLOR = colorOut;
	}
})";

	std::string_view fragDefsSrc = support.hasTextureSwizzle ? "" : "#define alphaTexSwizzle\n";
	std::array<std::string_view, 2> fragSrcs{fragDefsSrc, fragShaderSrc};
	if(!basicEffect_.setShaders(rTask, {&vertShaderSrc, 1}, fragSrcs)) [[unlikely]]
		return false;
	releaseShaderCompilerEvent.notify();
	return true;
}

BasicEffect &Renderer::basicEffect()
{
	return basicEffect_;
}

void Renderer::animateWindowRotation(Window &win, float srcAngle, float destAngle)
{
	winData(win).projAngleM = {srcAngle, destAngle, {}, SteadyClock::now(), Milliseconds{165}};
	win.addOnFrame([&win](FrameParams params)
	{
		win.signalSurfaceChanged({.contentRectResized = true});
		bool didUpdate = winData(win).projAngleM.update(params.timestamp);
		return didUpdate;
	});
}

float Renderer::projectionRollAngle(const Window &win) const
{
	return winData(win).projAngleM;
}

Texture Renderer::makeTexture(TextureConfig config)
{
	return {task(), config};
}

Texture Renderer::makeTexture(Data::PixmapSource img, TextureSamplerConfig samplerConf, bool makeMipmaps)
{
	return {task(), img, samplerConf, makeMipmaps};
}

PixmapBufferTexture Renderer::makePixmapBufferTexture(TextureConfig config, TextureBufferMode mode, bool singleBuffer)
{
	return {task(), config, mode, singleBuffer};
}

TextureSampler Renderer::makeTextureSampler(TextureSamplerConfig config)
{
	return {task(), config};
}

void destroyGLBuffer(RendererTask &task, NativeBuffer buff)
{
	logMsg("deleting GL buffer:%u", buff);
	task.run(
		[buff]()
		{
			glDeleteBuffers(1, &buff);
		});
}

const IndexBuffer<uint8_t> &rendererQuadIndices(const RendererTask &rTask) { return rTask.renderer().quadIndices; }

void Renderer::setCorrectnessChecks(bool on)
{
	if(on)
	{
		log.warn("enabling verification of OpenGL state");
	}
	GLStateCache::verifyState = on;
	checkGLErrors = on ? true : Config::DEBUG_BUILD;
	checkGLErrorsVerbose = on;
}

static void printFeatures(DrawContextSupport support)
{
	if constexpr(!Config::DEBUG_BUILD)
		return;
	std::string featuresStr{};
	featuresStr.reserve(256);

	featuresStr.append(" [Texture Size:");
	featuresStr.append(std::format("{}", support.textureSizeSupport.maxXSize));
	featuresStr.append("]");
	if(support.textureSizeSupport.nonPow2CanRepeat)
		featuresStr.append(" [NPOT Textures w/ Mipmap+Repeat]");
	else if(support.textureSizeSupport.nonPow2CanMipmap)
		featuresStr.append(" [NPOT Textures w/ Mipmap]");
	#ifdef CONFIG_GFX_OPENGL_ES
	if(support.hasBGRPixels)
	{
		featuresStr.append(" [BGRA Format]");
	}
	#endif
	if(Config::Gfx::OPENGL_ES && support.hasTextureSwizzle)
	{
		featuresStr.append(" [Texture Swizzle]");
	}
	if(support.hasImmutableTexStorage)
	{
		featuresStr.append(" [Immutable Texture Storage]");
	}
	if(support.hasImmutableBufferStorage())
	{
		featuresStr.append(" [Immutable Buffer Storage]");
	}
	if(support.hasMemoryBarriers())
	{
		featuresStr.append(" [Memory Barriers]");
	}
	if(Config::Gfx::OPENGL_ES && support.hasUnpackRowLength)
	{
		featuresStr.append(" [Unpack Sub-Images]");
	}
	if(Config::Gfx::OPENGL_ES && support.hasSamplerObjects)
	{
		featuresStr.append(" [Sampler Objects]");
	}
	if(Config::Gfx::OPENGL_ES && support.hasVAOFuncs())
	{
		featuresStr.append(" [VAOs]");
	}
	if(Config::Gfx::OPENGL_ES && support.hasPBOFuncs)
	{
		featuresStr.append(" [PBOs]");
	}
	if(!Config::Gfx::OPENGL_ES || (Config::Gfx::OPENGL_ES && (bool)support.glMapBufferRange))
	{
		featuresStr.append(" [Map Buffer Range]");
	}
	if(support.hasSyncFences())
	{
		if(Config::GL_PLATFORM_EGL)
		{
			if(support.hasServerWaitSync())
				featuresStr.append(" [EGL Sync Fences + Server Sync]");
			else
				featuresStr.append(" [EGL Sync Fences]");
		}
		else
		{
			featuresStr.append(" [Sync Fences]");
		}
	}
	if(Config::Gfx::OPENGL_ES && support.hasSrgbWriteControl)
	{
		featuresStr.append(" [sRGB FB Write Control]");
	}
	featuresStr.append(" [GLSL:");
	featuresStr.append((const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
	featuresStr.append("]");

	logMsg("features:%s", featuresStr.c_str());
}

#ifdef __ANDROID__
EGLImageKHR makeAndroidNativeBufferEGLImage(EGLDisplay dpy, EGLClientBuffer clientBuff, bool srgb)
{
	EGLint eglImgAttrs[]
	{
		EGL_IMAGE_PRESERVED_KHR,
		EGL_TRUE,
		srgb ? EGL_GL_COLORSPACE : EGL_NONE,
		srgb ? EGL_GL_COLORSPACE_SRGB : EGL_NONE,
		EGL_NONE
	};
	return eglCreateImageKHR(dpy, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID,
		clientBuff, eglImgAttrs);
}
#endif

void GLRenderer::setupNonPow2MipmapRepeatTextures()
{
	support.textureSizeSupport.nonPow2CanMipmap = true;
	support.textureSizeSupport.nonPow2CanRepeat = true;
}

void GLRenderer::setupImmutableTexStorage([[maybe_unused]] bool extSuffix)
{
	if(support.hasImmutableTexStorage)
		return;
	support.hasImmutableTexStorage = true;
	#ifdef CONFIG_GFX_OPENGL_ES
	const char *procName = extSuffix ? "glTexStorage2DEXT" : "glTexStorage2D";
	support.glTexStorage2D = (typeof(support.glTexStorage2D))glManager.procAddress(procName);
	#endif
}

void GLRenderer::setupRGFormats()
{
	support.luminanceFormat = GL_RED;
	support.luminanceInternalFormat = GL_R8;
	support.luminanceAlphaFormat = GL_RG;
	support.luminanceAlphaInternalFormat = GL_RG8;
	support.alphaFormat = GL_RED;
	support.alphaInternalFormat = GL_R8;
}

void GLRenderer::setupSamplerObjects()
{
	if(!Renderer::enableSamplerObjects || support.hasSamplerObjects)
		return;
	support.hasSamplerObjects = true;
	#ifdef CONFIG_GFX_OPENGL_ES
	support.glGenSamplers = (typeof(support.glGenSamplers))glManager.procAddress("glGenSamplers");
	support.glDeleteSamplers = (typeof(support.glDeleteSamplers))glManager.procAddress("glDeleteSamplers");
	support.glBindSampler = (typeof(support.glBindSampler))glManager.procAddress("glBindSampler");
	support.glSamplerParameteri = (typeof(support.glSamplerParameteri))glManager.procAddress("glSamplerParameteri");
	#endif
}

void GLRenderer::setupSpecifyDrawReadBuffers()
{
	#ifdef CONFIG_GFX_OPENGL_ES
	//support.glDrawBuffers = (typeof(support.glDrawBuffers))glManager.procAddress("glDrawBuffers");
	//support.glReadBuffer = (typeof(support.glReadBuffer))glManager.procAddress("glReadBuffer");
	#endif
}

void GLRenderer::setupUnmapBufferFunc()
{
	#ifdef CONFIG_GFX_OPENGL_ES
	if(!support.glUnmapBuffer)
	{
		if constexpr(Config::envIsAndroid || Config::envIsIOS)
		{
			support.glUnmapBuffer = (DrawContextSupport::UnmapBufferProto)glUnmapBufferOES;
		}
		else
		{
			if constexpr((bool)Config::Gfx::OPENGL_ES)
			{
				support.glUnmapBuffer = (typeof(support.glUnmapBuffer))glManager.procAddress("glUnmapBufferOES");
			}
			else
			{
				support.glUnmapBuffer = (typeof(support.glUnmapBuffer))glManager.procAddress("glUnmapBuffer");
			}
		}
	}
	#endif
}

void GLRenderer::setupImmutableBufferStorage()
{
	if(support.hasImmutableBufferStorage())
		return;
	#ifdef CONFIG_GFX_OPENGL_ES
	support.glBufferStorage = (typeof(support.glBufferStorage))glManager.procAddress("glBufferStorageEXT");
	#else
	support.hasBufferStorage = true;
	#endif
}

void GLRenderer::setupMemoryBarrier()
{
	/*if(support.hasMemoryBarriers())
		return;
	#ifdef CONFIG_GFX_OPENGL_ES
	support.glMemoryBarrier = (typeof(support.glMemoryBarrier))glManager.procAddress("glMemoryBarrier");
	#else
	support.hasMemoryBarrier = true;
	#endif*/
}

void GLRenderer::setupVAOFuncs([[maybe_unused]] bool oes)
{
	#ifdef CONFIG_GFX_OPENGL_ES
	if(support.glBindVertexArray)
		return;
	if(oes)
	{
		support.glBindVertexArray = (typeof(support.glBindVertexArray))glManager.procAddress("glBindVertexArrayOES");
		support.glGenVertexArrays = (typeof(support.glGenVertexArrays))glManager.procAddress("glGenVertexArraysOES");
		support.glDeleteVertexArrays = (typeof(support.glDeleteVertexArrays))glManager.procAddress("glDeleteVertexArraysOES");
	}
	else
	{
		support.glBindVertexArray = (typeof(support.glBindVertexArray))glManager.procAddress("glBindVertexArray");
		support.glGenVertexArrays = (typeof(support.glGenVertexArrays))glManager.procAddress("glGenVertexArrays");
		support.glDeleteVertexArrays = (typeof(support.glDeleteVertexArrays))glManager.procAddress("glDeleteVertexArrays");
	}
	#endif
}

void GLRenderer::setupFenceSync()
{
	#if !defined CONFIG_BASE_GL_PLATFORM_EGL && defined CONFIG_GFX_OPENGL_ES
	if(support.hasSyncFences())
		return;
	glManager.loadSymbol(support.glFenceSync, "glFenceSync");
	glManager.loadSymbol(support.glDeleteSync, "glDeleteSync");
	glManager.loadSymbol(support.glClientWaitSync, "glClientWaitSync");
	//glManager.loadSymbol(support.glWaitSync, "glWaitSync");
	#endif
}

void GLRenderer::setupAppleFenceSync()
{
	#if !defined CONFIG_BASE_GL_PLATFORM_EGL && defined CONFIG_GFX_OPENGL_ES
	if(support.hasSyncFences())
		return;
	glManager.loadSymbol(support.glFenceSync, "glFenceSyncAPPLE");
	glManager.loadSymbol(support.glDeleteSync, "glDeleteSyncAPPLE");
	glManager.loadSymbol(support.glClientWaitSync, "glClientWaitSyncAPPLE");
	//glManager.loadSymbol(support.glWaitSync, "glWaitSyncAPPLE");
	#endif
}

void GLRenderer::setupEglFenceSync([[maybe_unused]] std::string_view eglExtenstionStr)
{
	if(Config::MACHINE_IS_PANDORA)	// TODO: driver waits for full timeout even if commands complete,
		return;												// possibly broken glFlush() behavior?
	if(support.hasSyncFences())
		return;
	#if defined CONFIG_BASE_GL_PLATFORM_EGL && defined CONFIG_GFX_OPENGL_ES
	// check for fence sync via EGL extensions
	if(eglExtenstionStr.contains("EGL_KHR_fence_sync"))
	{
		glManager.loadSymbol(support.eglCreateSync, "eglCreateSyncKHR");
		glManager.loadSymbol(support.eglDestroySync, "eglDestroySyncKHR");
		glManager.loadSymbol(support.eglClientWaitSync, "eglClientWaitSyncKHR");
		/*if(eglExtenstionStr.contains("EGL_KHR_wait_sync"))
		{
			glManager.loadSymbol(support.eglWaitSync, "eglWaitSyncKHR");
		}*/
	}
	#endif
}

void GLRenderer::checkExtensionString(std::string_view extStr)
{
	//logMsg("checking %s", extStr);
	if(Config::DEBUG_BUILD && Config::OpenGLDebugContext && extStr == "GL_KHR_debug")
	{
		support.hasDebugOutput = true;
		#ifdef __ANDROID__
		// older GPU drivers like Tegra 3 can crash when using debug output,
		// only enable on recent Android version to be safe
		if(mainTask.appContext().androidSDK() < 23)
		{
			support.hasDebugOutput = false;
		}
		#endif
	}
	#ifdef CONFIG_GFX_OPENGL_ES
	else if(extStr == "GL_OES_texture_npot")
	{
		// allows mipmaps and repeat modes
		setupNonPow2MipmapRepeatTextures();
	}
	else if(!Config::envIsIOS && extStr == "GL_NV_texture_npot_2D_mipmap")
	{
		// no repeat modes
		support.textureSizeSupport.nonPow2CanMipmap = true;
	}
	else if(extStr == "GL_EXT_unpack_subimage")
	{
		support.hasUnpackRowLength = true;
	}
	else if((!Config::envIsIOS && extStr == "GL_EXT_texture_format_BGRA8888")
			|| (Config::envIsIOS && extStr == "GL_APPLE_texture_format_BGRA8888"))
	{
		support.hasBGRPixels = true;
	}
	else if(extStr == "GL_EXT_texture_storage")
	{
		setupImmutableTexStorage(true);
	}
	else if(!Config::GL_PLATFORM_EGL && Config::envIsIOS && extStr == "GL_APPLE_sync")
	{
		setupAppleFenceSync();
	}
	else if(Config::envIsAndroid && extStr == "GL_OES_EGL_image")
	{
		support.hasEGLImages = true;
	}
	else if(Config::Gfx::OPENGL_TEXTURE_TARGET_EXTERNAL &&
		extStr == "GL_OES_EGL_image_external")
	{
		support.hasExternalEGLImages = true;
	}
	#ifdef __ANDROID__
	else if(extStr == "GL_EXT_EGL_image_storage")
	{
		support.glEGLImageTargetTexStorageEXT = (typeof(support.glEGLImageTargetTexStorageEXT))glManager.procAddress("glEGLImageTargetTexStorageEXT");
	}
	#endif
	else if(extStr == "GL_NV_pixel_buffer_object")
	{
		support.hasPBOFuncs = true;
	}
	else if(extStr == "GL_NV_map_buffer_range")
	{
		if(!support.glMapBufferRange)
			support.glMapBufferRange = (typeof(support.glMapBufferRange))glManager.procAddress("glMapBufferRangeNV");
		setupUnmapBufferFunc();
	}
	else if(extStr == "GL_EXT_map_buffer_range")
	{
		if(!support.glMapBufferRange)
			support.glMapBufferRange = (typeof(support.glMapBufferRange))glManager.procAddress("glMapBufferRangeEXT");
		// Only using ES 3.0 version currently
		//if(!support.glFlushMappedBufferRange)
		//	support.glFlushMappedBufferRange = (typeof(support.glFlushMappedBufferRange))glManager.procAddress("glFlushMappedBufferRangeEXT");
		setupUnmapBufferFunc();
	}
	else if(extStr == "GL_EXT_buffer_storage")
	{
		setupImmutableBufferStorage();
	}
	/*else if(string_equal(extStr, "GL_OES_mapbuffer"))
	{
		// handled in *_map_buffer_range currently
	}*/
	else if(extStr == "GL_EXT_sRGB_write_control")
	{
		support.hasSrgbWriteControl = true;
	}
	else if(extStr == "GL_OES_vertex_array_object"
		&& !Config::MACHINE_IS_PANDORA) // VAOs may crash inside GL driver on Pandora
	{
		setupVAOFuncs(true);
	}
	#endif
	#ifndef CONFIG_GFX_OPENGL_ES
	/*else if(string_equal(extStr, "GL_EXT_texture_filter_anisotropic"))
	{
		setupAnisotropicFiltering();
	}
	else if(string_equal(extStr, "GL_ARB_multisample"))
	{
		setupMultisample();
	}
	else if(string_equal(extStr, "GL_NV_multisample_filter_hint"))
	{
		setupMultisampleHints();
	}*/
	else if(extStr == "GL_ARB_texture_storage")
	{
		setupImmutableTexStorage(false);
	}
	else if(extStr == "GL_ARB_buffer_storage")
	{
		setupImmutableBufferStorage();
	}
	else if(extStr == "GL_ARB_shader_image_load_store")
	{
		setupMemoryBarrier();
	}
	#endif
}

static void printGLExtensions()
{
	if constexpr(!Config::DEBUG_BUILD)
		return;
	std::string allExtStr;
	log.beginInfo(allExtStr, "extensions: ");
	forEachOpenGLExtension([&](const auto &extStr)
	{
		allExtStr += extStr;
		allExtStr += ' ';
	});
	log.printInfo(allExtStr);
}

void Renderer::configureRenderer()
{
	if(Config::DEBUG_BUILD && defaultToFullErrorChecks)
	{
		setCorrectnessChecks(true);
	}
	task().runSync(
		[this](GLTask::TaskContext ctx)
		{
			auto version = (const char*)glGetString(GL_VERSION);
			assert(version);
			auto rendererName = (const char*)glGetString(GL_RENDERER);
			logMsg("version: %s (%s)", version, rendererName);

			int glVer = glVersionFromStr(version);

			#ifdef CONFIG_BASE_GL_PLATFORM_EGL
			if constexpr((bool)Config::Gfx::OPENGL_ES)
			{
				auto extStr = ctx.glDisplay.queryExtensions();
				setupEglFenceSync(extStr);
			}
			#endif

			#ifndef CONFIG_GFX_OPENGL_ES
			assert(glVer >= 33);
			#else
			// core functionality
			assumeExpr(glVer >= 20);
			if(glVer >= 30)
				setupNonPow2MipmapRepeatTextures();
			if(glVer >= 30)
			{
				support.glMapBufferRange = (typeof(support.glMapBufferRange))glManager.procAddress("glMapBufferRange");
				support.glUnmapBuffer = (typeof(support.glUnmapBuffer))glManager.procAddress("glUnmapBuffer");
				support.glFlushMappedBufferRange = (typeof(support.glFlushMappedBufferRange))glManager.procAddress("glFlushMappedBufferRange");
				setupImmutableTexStorage(false);
				support.hasTextureSwizzle = true;
				setupRGFormats();
				setupSamplerObjects();
				support.hasPBOFuncs = true;
				setupVAOFuncs();
				if(!Config::GL_PLATFORM_EGL)
					setupFenceSync();
				if(!Config::envIsIOS)
					setupSpecifyDrawReadBuffers();
				support.hasUnpackRowLength = true;
				support.useLegacyGLSL = false;
			}
			if(glVer >= 31)
			{
				setupMemoryBarrier();
			}
			#endif // CONFIG_GFX_OPENGL_ES

			// extension functionality
			forEachOpenGLExtension([&](const auto &extStr)
			{
				checkExtensionString(extStr);
			});
			printGLExtensions();

			GLint texSize;
			glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texSize);
			support.textureSizeSupport.maxXSize = support.textureSizeSupport.maxYSize = texSize;
			assert(support.textureSizeSupport.maxXSize > 0 && support.textureSizeSupport.maxYSize > 0);

			printFeatures(support);
			task().runInitialCommandsInGL(ctx, support);
		});
	support.isConfigured = true;
}

bool Renderer::isConfigured() const
{
	return support.isConfigured;
}

const RendererTask &Renderer::task() const
{
	return mainTask;
}

RendererTask &Renderer::task()
{
	return mainTask;
}

void Renderer::setWindowValidOrientations(Window &win, Orientations validO)
{
	if(!win.isMainWindow())
		return;
	auto oldWinO = win.softOrientation();
	if(win.setValidOrientations(validO) && !Config::SYSTEM_ROTATES_WINDOWS)
	{
		animateWindowRotation(win, rotationRadians(oldWinO), rotationRadians(win.softOrientation()));
	}
}

void GLRenderer::addEventHandlers(ApplicationContext, RendererTask &task)
{
	releaseShaderCompilerEvent.attach();
	if constexpr(Config::envIsIOS)
		task.setIOSDrawableDelegates();
}

std::optional<GLBufferConfig> GLRenderer::makeGLBufferConfig(ApplicationContext ctx, IG::PixelFormat pixelFormat, const Window *winPtr)
{
	if(!pixelFormat)
	{
		if(winPtr)
			pixelFormat = winPtr->pixelFormat();
		else
			pixelFormat = ctx.defaultWindowPixelFormat();
	}
	try
	{
		if constexpr(Config::Gfx::OPENGL_ES)
		{
			// prefer ES 3.x and fall back to 2.0
			const GLBufferRenderConfigAttributes gl3Attrs{.bufferAttrs{.pixelFormat = pixelFormat}, .version = {3}, .api = glAPI};
			const GLBufferRenderConfigAttributes gl2Attrs{.bufferAttrs{.pixelFormat = pixelFormat}, .version = {2}, .api = glAPI};
			return glManager.makeBufferConfig(ctx, std::array{gl3Attrs, gl2Attrs});
		}
		else
		{
			// full OpenGL
			const GLBufferRenderConfigAttributes gl3Attrs{.bufferAttrs{.pixelFormat = pixelFormat}, .version = {3, 3}, .api = glAPI};
			return glManager.makeBufferConfig(ctx, gl3Attrs);
		}
	}
	catch(...) { return {}; }
}

}
