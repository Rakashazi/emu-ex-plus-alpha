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
#include "internalDefs.hh"
#ifdef __ANDROID__
#include <imagine/base/platformExtras.hh>
#endif

namespace Gfx
{

Renderer::Renderer(RendererConfig config, Error &err): GLRenderer{Init{}}
{
	auto [ec, dpy] = Base::GLDisplay::makeDefault(glAPI);
	if(ec)
	{
		logErr("error getting GL display");
		err = std::runtime_error("error creating GL display");
		return;
	}
	glDpy = dpy;
	dpy.logInfo();
	{
		auto [mainContext, ec] = makeGLContext(dpy, config.pixelFormat());
		if(!mainContext)
		{
			err = std::runtime_error("error creating GL context");
			return;
		}
		constexpr int DRAW_THREAD_PRIORITY = -4;
		mainTask = std::make_unique<RendererTask>("Main GL Context Messages", *this, mainContext, DRAW_THREAD_PRIORITY);
	}
	addEventHandlers(*mainTask);
}

Renderer::Renderer(Error &err): Renderer({Base::Window::defaultPixelFormat()}, err) {}

Renderer::Renderer(Renderer &&o)
{
	*this = std::move(o);
}

Renderer &Renderer::operator=(Renderer &&o)
{
	deinit();
	GLRenderer::operator=(std::move(o));
	if(mainTask)
		mainTask->setRenderer(this);
	o.glDpy = {};
	return *this;
}

GLRenderer::GLRenderer(Init):
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

static Base::GLContextAttributes makeGLContextAttributes(uint32_t majorVersion, uint32_t minorVersion)
{
	Base::GLContextAttributes glAttr;
	if(Config::DEBUG_BUILD)
		glAttr.setDebug(true);
	glAttr.setMajorVersion(majorVersion);
	#ifdef CONFIG_GFX_OPENGL_ES
	glAttr.setOpenGLESAPI(true);
	#else
	glAttr.setMinorVersion(minorVersion);
	#endif
	return glAttr;
}

Base::GLContextAttributes GLRenderer::makeKnownGLContextAttributes()
{
	#ifdef CONFIG_GFX_OPENGL_ES
	if(Config::Gfx::OPENGL_ES == 1)
	{
		return makeGLContextAttributes(1, 0);
	}
	else
	{
		assert(glMajorVer);
		return makeGLContextAttributes(glMajorVer, 0);
	}
	#else
	if(Config::Gfx::OPENGL_SHADER_PIPELINE)
	{
		return makeGLContextAttributes(3, 3);
	}
	else
	{
		return makeGLContextAttributes(1, 3);
	}
	#endif
}

void GLRenderer::finishContextCreation(Base::GLContext ctx)
{
	#if CONFIG_GFX_OPENGL_ES_MAJOR_VERSION > 1
	if(Config::envIsAndroid && Config::MACHINE == Config::Machine::GENERIC_ARMV7 && glMajorVer == 2)
	{
		// Vivante "GC1000 core" GPU (Samsung Galaxy S3 Mini, Galaxy Tab 3), possibly others, will fail
		// setting context in render thread with EGL_BAD_ACCESS unless it's first set in the creation thread,
		// exact cause unknown and is most likely a driver bug
		logDMsg("toggling newly created context current on this thread to avoid driver issues");
		ctx.setCurrent(glDpy, ctx, {});
		ctx.setCurrent(glDpy, {}, {});
	}
	#endif
}

std::pair<Base::GLContext, IG::ErrorCode> GLRenderer::makeGLContext(Base::GLDisplay dpy, Base::GLBufferConfigAttributes glBuffAttr,
	unsigned majorVersion, unsigned minorVersion)
{
	auto glAttr = makeGLContextAttributes(majorVersion, minorVersion);
	auto [found, config] = Base::GLContext::makeBufferConfig(dpy, glAttr, glBuffAttr);
	if(!found)
	{
		return {{}, {EINVAL}};
	}
	gfxBufferConfig = config;
	glMajorVer = glAttr.majorVersion();
	IG::ErrorCode ec{};
	Base::GLContext glCtx{dpy, glAttr, config, ec};
	return {glCtx, ec};
}

std::pair<Base::GLContext, IG::ErrorCode> GLRenderer::makeGLContext(Base::GLDisplay dpy, IG::PixelFormat pixelFormat)
{
	IG::ErrorCode ec{};
	if(!pixelFormat)
		pixelFormat = Base::Window::defaultPixelFormat();
	Base::GLBufferConfigAttributes glBuffAttr;
	glBuffAttr.setPixelFormat(pixelFormat);
	std::pair<Base::GLContext, IG::ErrorCode> ctxWithEC{};
	if constexpr(Config::Gfx::OPENGL_ES == 1)
	{
		ctxWithEC = makeGLContext(dpy, glBuffAttr, 1, 0);
	}
	else if constexpr(Config::Gfx::OPENGL_ES >= 2)
	{
		constexpr bool CAN_USE_OPENGL_ES_3 = !Config::MACHINE_IS_PANDORA;
		if(CAN_USE_OPENGL_ES_3)
		{
			ctxWithEC = makeGLContext(dpy, glBuffAttr, 3, 0);
		}
		if(!std::get<Base::GLContext>(ctxWithEC))
		{
			// fall back to OpenGL ES 2.0
			ctxWithEC = makeGLContext(dpy, glBuffAttr, 2, 0);
		}
	}
	else
	{
		if(Config::Gfx::OPENGL_SHADER_PIPELINE)
		{
			support.useFixedFunctionPipeline = false;
			ctxWithEC = makeGLContext(dpy, glBuffAttr, 3, 3);
		}
		if(Config::Gfx::OPENGL_FIXED_FUNCTION_PIPELINE && !std::get<Base::GLContext>(ctxWithEC))
		{
			// fall back to OpenGL 1.3
			support.useFixedFunctionPipeline = true;
			ctxWithEC = makeGLContext(dpy, glBuffAttr, 1, 3);
		}
	}
	auto glCtx = std::get<Base::GLContext>(ctxWithEC);
	if(unlikely(!glCtx))
		return ctxWithEC;
	finishContextCreation(glCtx);
	return ctxWithEC;
}

std::pair<Base::GLContext, IG::ErrorCode> GLRenderer::makeGLContextWithKnownConfig(Base::GLDisplay dpy, Base::GLContext shareContext)
{
	assert(mainTask->glContext());
	IG::ErrorCode ec{};
	Base::GLContext glCtx{dpy, makeKnownGLContextAttributes(), gfxBufferConfig, shareContext, ec};
	finishContextCreation(glCtx);
	return {glCtx, ec};
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

void Renderer::setPresentationTime(Drawable drawable, IG::FrameTime time) const
{
	#ifdef __ANDROID__
	if(!support.eglPresentationTimeANDROID)
		return;
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

}
