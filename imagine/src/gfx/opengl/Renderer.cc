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
	GLRenderer{ctx}
{
	auto [ec, dpy] = Base::GLDisplay::makeDefault(ctx.nativeDisplayConnection(), glAPI);
	if(ec)
	{
		err = std::runtime_error("error creating GL display connection");
		return;
	}
	dpy.logInfo();
	glDpy = dpy;
}

GLRenderer::GLRenderer(Base::ApplicationContext ctx):
	mainTask{ctx, "Main GL Context Messages", *static_cast<Renderer*>(this)},
	releaseShaderCompilerEvent{"GLRenderer::releaseShaderCompilerEvent"}
{}

Error Renderer::setPixelFormat(IG::PixelFormat format)
{
	auto ctx = appContext();
	if(format == PIXEL_FMT_NONE)
		format = Base::Window::defaultPixelFormat(ctx);
	auto bufferConfig = makeGLBufferConfig(ctx, format);
	if(unlikely(!bufferConfig))
	{
		return std::runtime_error("error finding a GL configuration");
	}
	gfxBufferConfig = *bufferConfig;
	return {};
}

Error Renderer::initMainTask(Base::Window *initialWindow, IG::PixelFormat format)
{
	if(auto err = setPixelFormat(format);
		unlikely(err))
	{
		return err;
	}
	if(mainTask.glContext())
	{
		return {};
	}
	Drawable initialDrawable{};
	if(initialWindow)
	{
		if(!attachWindow(*initialWindow))
		{
			return std::runtime_error("error creating window surface");
		}
		initialDrawable = (Drawable)winData(*initialWindow).drawableHolder;
	}
	constexpr int DRAW_THREAD_PRIORITY = -4;
	GLTaskConfig conf
	{
		.display = glDisplay(),
		.bufferConfig = gfxBufferConfig,
		.initialDrawable = initialDrawable,
		.threadPriority = DRAW_THREAD_PRIORITY,
	};
	if(auto err = mainTask.makeGLContext(conf);
		unlikely(err))
	{
		return err;
	}
	mainTask.setDrawAsyncMode(maxSwapChainImages() < 3 ? DrawAsyncMode::PRESENT : DrawAsyncMode::NONE);
	addEventHandlers(appContext(), mainTask);
	configureRenderer();
	return {};
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
	auto &rData = win.makeRendererData<GLRendererWindowData>(win.appContext());
	rData.drawableHolder.makeDrawable(glDisplay(), win, gfxBufferConfig);
	if(unlikely(!rData.drawableHolder))
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
						animateProjectionMatrixRotation(win, orientationToGC(oldWinO), orientationToGC(newO));
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
					animateProjectionMatrixRotation(win, rotAngle, 0.);
				});
		}
	}
	return true;
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
	auto drawable = (Drawable)winData(win).drawableHolder;
	bool success = support.eglPresentationTimeANDROID(glDisplay(), drawable, time.count());
	if(Config::DEBUG_BUILD && !success)
	{
		logErr("error:%s in eglPresentationTimeANDROID(%p, %llu)",
			glDisplay().errorString(eglGetError()), (EGLSurface)drawable, (unsigned long long)time.count());
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
	return glDpy;
}

GLDisplayHolder::~GLDisplayHolder()
{
	dpy.deinit();
}

GLDisplayHolder::GLDisplayHolder(GLDisplayHolder &&o)
{
	*this = std::move(o);
}

GLDisplayHolder &GLDisplayHolder::operator=(GLDisplayHolder &&o)
{
	dpy.deinit();
	dpy = std::exchange(o.dpy, {});
	return *this;
}

}
