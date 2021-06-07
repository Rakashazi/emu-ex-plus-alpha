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
#include <imagine/logger/logger.h>
#include <imagine/base/Window.hh>
#include <imagine/base/GLContext.hh>
#include <imagine/base/ApplicationContext.hh>
#include "internalDefs.hh"

namespace Gfx
{

Renderer::Renderer(Base::ApplicationContext ctx, Error &err):
	GLRenderer{ctx, err}
{}

Renderer::~Renderer()
{
	for(auto &w : appContext().windows())
	{
		detachWindow(*w);
	}
}

GLRenderer::GLRenderer(Base::ApplicationContext ctx, Error &err):
	glManager{ctx.nativeDisplayConnection(), glAPI},
	mainTask{ctx, "Main GL Context Messages", *static_cast<Renderer*>(this)},
	releaseShaderCompilerEvent{"GLRenderer::releaseShaderCompilerEvent"}
{
	if(!glManager)
	{
		err = std::runtime_error("error getting GL display");
		return;
	}
	glManager.logInfo();
}

Error Renderer::initMainTask(Base::Window *initialWindow, DrawableConfig drawableConfig)
{
	if(mainTask.glContext())
	{
		return {};
	}
	auto ctx = appContext();
	auto bufferConfig = makeGLBufferConfig(ctx, drawableConfig.pixelFormat, initialWindow);
	if(!bufferConfig) [[unlikely]]
	{
		return std::runtime_error("error finding a GL configuration");
	}
	Drawable initialDrawable{};
	if(initialWindow)
	{
		if(!GLRenderer::attachWindow(*initialWindow, *bufferConfig, (Base::GLColorSpace)drawableConfig.colorSpace))
		{
			return std::runtime_error("error creating window surface");
		}
		initialDrawable = (Drawable)winData(*initialWindow).drawable;
	}
	constexpr int DRAW_THREAD_PRIORITY = -4;
	GLTaskConfig conf
	{
		.glManagerPtr = &glManager,
		.bufferConfig = *bufferConfig,
		.initialDrawable = initialDrawable,
		.threadPriority = DRAW_THREAD_PRIORITY,
	};
	if(auto err = mainTask.makeGLContext(conf);
		err) [[unlikely]]
	{
		return err;
	}
	addEventHandlers(ctx, mainTask);
	configureRenderer();
	return {};
}

Base::NativeWindowFormat GLRenderer::nativeWindowFormat(Base::GLBufferConfig bufferConfig) const
{
	return glManager.nativeWindowFormat(mainTask.appContext(), bufferConfig);
}

bool GLRenderer::attachWindow(Base::Window &win, Base::GLBufferConfig bufferConfig, Base::GLColorSpace colorSpace)
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
			rData.projAngleM = orientationToGC(win.softOrientation());
			win.appContext().setOnDeviceOrientationChanged(
				[this, &win](Base::ApplicationContext, Base::Orientation newO)
				{
					auto oldWinO = win.softOrientation();
					if(win.requestOrientationChange(newO))
					{
						static_cast<Renderer*>(this)->animateProjectionMatrixRotation(win, orientationToGC(oldWinO), orientationToGC(newO));
					}
				});
		}
		else if(Config::SYSTEM_ROTATES_WINDOWS && !win.appContext().systemAnimatesWindowRotation())
		{
			win.appContext().setOnSystemOrientationChanged(
				[this, &win](Base::ApplicationContext, Base::Orientation oldO, Base::Orientation newO) // TODO: parameters need proper type definitions in API
				{
					const Angle orientationDiffTable[4][4]
					{
						{0, angleFromDegree(90), angleFromDegree(-180), angleFromDegree(-90)},
						{angleFromDegree(-90), 0, angleFromDegree(90), angleFromDegree(-180)},
						{angleFromDegree(-180), angleFromDegree(-90), 0, angleFromDegree(90)},
						{angleFromDegree(90), angleFromDegree(-180), angleFromDegree(-90), 0},
					};
					auto rotAngle = orientationDiffTable[oldO][newO];
					logMsg("animating from %d degrees", (int)angleToDegree(rotAngle));
					static_cast<Renderer*>(this)->animateProjectionMatrixRotation(win, rotAngle, 0.);
				});
		}
	}
	return true;
}

bool GLRenderer::makeWindowDrawable(RendererTask &task, Base::Window &win, Base::GLBufferConfig bufferConfig, Base::GLColorSpace colorSpace)
{
	auto &rData = winData(win);
	rData.bufferConfig = bufferConfig;
	rData.colorSpace = colorSpace;
	task.destroyDrawable(rData.drawable);
	Base::GLDrawableAttributes attr{bufferConfig};
	attr.setColorSpace(colorSpace);
	IG::ErrorCode ec{};
	rData.drawable = glManager.makeDrawable(win, attr, ec);
	if(ec) [[unlikely]]
	{
		return false;
	}
	return true;
}

bool Renderer::attachWindow(Base::Window &win, DrawableConfig drawableConfig)
{
	Base::GLBufferConfig bufferConfig = mainTask.glBufferConfig();
	if(canRenderToMultiplePixelFormats())
	{
		auto bufferConfigOpt = makeGLBufferConfig(appContext(), drawableConfig.pixelFormat, &win);
		if(!bufferConfigOpt) [[unlikely]]
		{
			return false;
		}
		bufferConfig = *bufferConfigOpt;
	}
	return GLRenderer::attachWindow(win, bufferConfig, (Base::GLColorSpace)drawableConfig.colorSpace);
}

void Renderer::detachWindow(Base::Window &win)
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

bool Renderer::setDrawableConfig(Base::Window &win, DrawableConfig config)
{
	auto bufferConfig = makeGLBufferConfig(appContext(), config.pixelFormat, &win);
	if(!bufferConfig) [[unlikely]]
	{
		return false;
	}
	if(winData(win).bufferConfig == *bufferConfig && winData(win).colorSpace == (Base::GLColorSpace)config.colorSpace)
	{
		return true;
	}
	if(mainTask.glBufferConfig() != *bufferConfig && !canRenderToMultiplePixelFormats())
	{
		// context only supports config it was created with
		return false;
	}
	win.setFormat(config.pixelFormat);
	return makeWindowDrawable(mainTask, win, *bufferConfig, (Base::GLColorSpace)config.colorSpace);
}

bool Renderer::canRenderToMultiplePixelFormats() const
{
	return glManager.hasNoConfigContext();
}

void Renderer::releaseShaderCompiler()
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	task().releaseShaderCompiler();
	#endif
}

void Renderer::autoReleaseShaderCompiler()
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	releaseShaderCompilerEvent.notify();
	#endif
}

ClipRect Renderer::makeClipRect(const Base::Window &win, IG::WindowRect rect)
{
	int x = rect.x;
	int y = rect.y;
	int w = rect.xSize();
	int h = rect.ySize();
	//logMsg("scissor before transform %d,%d size %d,%d", x, y, w, h);
	// translate from view to window coordinates
	if(!Config::SYSTEM_ROTATES_WINDOWS)
	{
		using namespace Base;
		switch(win.softOrientation())
		{
			bcase VIEW_ROTATE_0:
				//x += win.viewport.rect.x;
				y = win.height() - (y + h);
			bcase VIEW_ROTATE_90:
				//x += win.viewport.rect.y;
				//y = win.width() - (y + h /*+ (win.w - win.viewport.rect.x2)*/);
				std::swap(x, y);
				std::swap(w, h);
				x = (win.realWidth() - x) - w;
				y = (win.realHeight() - y) - h;
			bcase VIEW_ROTATE_270:
				//x += win.viewport.rect.y;
				//y += win.viewport.rect.x;
				std::swap(x, y);
				std::swap(w, h);
			bcase VIEW_ROTATE_180:
				x = (win.realWidth() - x) - w;
				//y = win.height() - (y + h);
				//std::swap(x, y);
				//std::swap(w, h);
				//x += win.viewport.rect.x;
				//y += win.height() - win.viewport.bounds().y2;
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

void Renderer::setPresentationTime(Base::Window &win, IG::FrameTime time) const
{
	#ifdef __ANDROID__
	if(!support.eglPresentationTimeANDROID)
		return;
	auto drawable = (Drawable)winData(win).drawable;
	bool success = support.eglPresentationTimeANDROID(glDisplay(), drawable, time.count());
	if(Config::DEBUG_BUILD && !success)
	{
		logErr("error:%s in eglPresentationTimeANDROID(%p, %llu)",
			Base::GLManager::errorString(eglGetError()), (EGLSurface)drawable, (unsigned long long)time.count());
	}
	#endif
}

unsigned Renderer::maxSwapChainImages() const
{
	#ifdef __ANDROID__
	if(appContext().androidSDK() < 18)
		return 2;
	#endif
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

Base::ApplicationContext Renderer::appContext() const
{
	return task().appContext();
}

GLRendererWindowData &winData(Base::Window &win)
{
	assumeExpr(win.rendererData<GLRendererWindowData>());
	return *win.rendererData<GLRendererWindowData>();
}

Base::GLDisplay GLRenderer::glDisplay() const
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
			.config{ .pixelFormat = PIXEL_RGBA8888, .colorSpace{} }
		},
		{
			.name = "RGBA8888:sRGB",
			.config{ .pixelFormat = PIXEL_RGBA8888, .colorSpace = ColorSpace::SRGB }
		},
		{
			.name = "RGB565",
			.config{ .pixelFormat = PIXEL_RGB565, .colorSpace{} }
		},
	};
	for(auto desc : testDescs)
	{
		if(glManager.hasDrawableConfig(desc.config.pixelFormat, (Base::GLColorSpace)desc.config.colorSpace))
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

}
