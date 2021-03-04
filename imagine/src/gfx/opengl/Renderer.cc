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
#include <imagine/base/Base.hh>
#include "internalDefs.hh"
#ifdef __ANDROID__
#include <imagine/base/platformExtras.hh>
#endif

namespace Gfx
{

Renderer::Renderer(RendererConfig config, Base::Window *initialWindow, Error &err)
{
	auto pixelFormat = config.pixelFormat();
	if(pixelFormat == PIXEL_FMT_NONE)
		pixelFormat = Base::Window::defaultPixelFormat();
	{
		auto [ec, dpy] = Base::GLDisplay::makeDefault(glAPI);
		if(ec)
		{
			err = std::runtime_error("error creating GL display connection");
			return;
		}
		glDpy = dpy;
	}
	{
		auto bufferConfig = makeGLBufferConfig(pixelFormat);
		if(unlikely(!bufferConfig))
		{
			err = std::runtime_error("error finding a GL configuration");
			return;
		}
		gfxBufferConfig = *bufferConfig;
	}
	glDpy.logInfo();
	Drawable initialDrawable{};
	if(initialWindow)
	{
		if(!attachWindow(*initialWindow))
		{
			err = std::runtime_error("error creating window surface");
			return;
		}
		initialDrawable = (Drawable)winData(*initialWindow).drawableHolder;
	}
	constexpr int DRAW_THREAD_PRIORITY = -4;
	GLTaskConfig conf
	{
		.bufferConfig = gfxBufferConfig,
		.initialDrawable = initialDrawable,
		.threadPriority = DRAW_THREAD_PRIORITY,
	};
	err = mainTask.makeGLContext(conf);
	if(unlikely(err))
	{
		return;
	}
	mainTask.setDrawAsyncMode(maxSwapChainImages() < 3 ? DrawAsyncMode::PRESENT : DrawAsyncMode::NONE);
	addEventHandlers(mainTask);
	configureRenderer();
}

Renderer::Renderer(Base::Window *initialWindow, Error &err):
	Renderer({Base::Window::defaultPixelFormat()}, initialWindow, err)
{}

GLRenderer::GLRenderer():
	mainTask{"Main GL Context Messages", *static_cast<Renderer*>(this)},
	releaseShaderCompilerEvent{"GLRenderer::releaseShaderCompilerEvent"}
{}

GLRenderer::~GLRenderer()
{
	deinit();
}

void GLRenderer::deinit()
{
	glDpy.deinit();
}

bool Renderer::attachWindow(Base::Window &win)
{
	if(unlikely(!win.hasSurface()))
	{
		logMsg("can't attach uninitialized window");
		return false;
	}
	logMsg("attaching window:%p", &win);
	win.setFormat(nativeWindowFormat());
	auto &rData = win.makeRendererData<GLRendererWindowData>();
	rData.drawableHolder.makeDrawable(glDpy, win, gfxBufferConfig);
	if(unlikely(!rData.drawableHolder))
	{
		return false;
	}
	if(win == Base::mainWindow())
	{
		if(!Config::SYSTEM_ROTATES_WINDOWS)
		{
			rData.projAngleM = orientationToGC(win.softOrientation());
			Base::setOnDeviceOrientationChanged(
				[this, &win](Base::Orientation newO)
				{
					auto oldWinO = win.softOrientation();
					if(win.requestOrientationChange(newO))
					{
						animateProjectionMatrixRotation(win, orientationToGC(oldWinO), orientationToGC(newO));
					}
				});
		}
		else if(Config::SYSTEM_ROTATES_WINDOWS && !Base::Window::systemAnimatesRotation())
		{
			Base::setOnSystemOrientationChanged(
				[this, &win](Base::Orientation oldO, Base::Orientation newO) // TODO: parameters need proper type definitions in API
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
					animateProjectionMatrixRotation(win, rotAngle, 0.);
				});
		}
	}
	return true;
}

void Renderer::detachWindow(Base::Window &win)
{
	win.resetRendererData();
	if(win == Base::mainWindow())
	{
		if(!Config::SYSTEM_ROTATES_WINDOWS)
		{
			Base::setOnDeviceOrientationChanged({});
		}
		else if(Config::SYSTEM_ROTATES_WINDOWS && !Base::Window::systemAnimatesRotation())
		{
			Base::setOnSystemOrientationChanged({});
		}
	}
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
	return {x, y, w, h};
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
	auto drawable = (Drawable)winData(win).drawableHolder;
	bool success = support.eglPresentationTimeANDROID(glDpy, drawable, time.count());
	if(Config::DEBUG_BUILD && !success)
	{
		logErr("error:%s in eglPresentationTimeANDROID(%p, %llu)",
			glDpy.errorString(eglGetError()), (EGLSurface)drawable, (unsigned long long)time.count());
	}
	#endif
}

unsigned Renderer::maxSwapChainImages() const
{
	#ifdef __ANDROID__
	if(Base::androidSDK() < 18)
		return 2;
	#endif
	return 3; // assume triple-buffering by default
}

GLRendererWindowData &winData(Base::Window &win)
{
	assumeExpr(win.rendererData<GLRendererWindowData>());
	return *win.rendererData<GLRendererWindowData>();
}

}
