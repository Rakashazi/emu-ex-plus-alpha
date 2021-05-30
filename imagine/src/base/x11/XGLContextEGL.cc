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

#define LOGTAG "EGL"
#include <imagine/base/GLContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/time/Time.hh>
#include <imagine/logger/logger.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <X11/Xutil.h>

#ifndef EGL_PLATFORM_X11_EXT
#define EGL_PLATFORM_X11_EXT 0x31D5
#endif

namespace Base
{

static constexpr bool HAS_EGL_PLATFORM = Config::envIsLinux && !Config::MACHINE_IS_PANDORA;

GLDisplay GLManager::getDefaultDisplay(NativeDisplayConnection nativeDpy) const
{
	if constexpr(HAS_EGL_PLATFORM)
	{
		return {eglGetPlatformDisplay(EGL_PLATFORM_X11_EXT, nativeDpy, nullptr)};
	}
	else
	{
		return {eglGetDisplay(EGL_DEFAULT_DISPLAY)};
	}
}

bool GLManager::bindAPI(GL::API api)
{
	if(api == GL::API::OPENGL_ES)
		return eglBindAPI(EGL_OPENGL_ES_API);
	else
		return eglBindAPI(EGL_OPENGL_API);
}

std::optional<GLBufferConfig> GLManager::makeBufferConfig(Base::ApplicationContext, GLBufferConfigAttributes attr, GL::API api, unsigned majorVersion) const
{
	auto renderableType = makeRenderableType(api, majorVersion);
	return chooseConfig(display(), renderableType, attr);
}

Base::NativeWindowFormat GLManager::nativeWindowFormat(Base::ApplicationContext ctx, GLBufferConfig glConfig) const
{
	if(Config::MACHINE_IS_PANDORA)
		return nullptr;
	// get matching x visual
	EGLint nativeID;
	eglGetConfigAttrib(display(), glConfig, EGL_NATIVE_VISUAL_ID, &nativeID);
	XVisualInfo viTemplate{};
	viTemplate.visualid = nativeID;
	int visuals;
	auto viPtr = XGetVisualInfo(ctx.application().xDisplay(), VisualIDMask, &viTemplate, &visuals);
	if(!viPtr)
	{
		logErr("unable to find matching X Visual");
		return nullptr;
	}
	auto visual = viPtr->visual;
	XFree(viPtr);
	return visual;
}

bool GLManager::hasBufferConfig(GLBufferConfigAttributes attrs) const
{
	if(attrs.pixelFormat().id() == PIXEL_NONE)
		return true;
	auto dpy = display();
	auto configOpt = chooseConfig(dpy, 0, attrs, false);
	if(!configOpt)
		return false;
	// verify the returned config is a match
	auto eglConfigInt =
		[](EGLDisplay dpy, EGLConfig config, EGLint attr)
		{
			EGLint val;
			eglGetConfigAttrib(dpy, config, attr, &val);
			return val;
		};
	switch(attrs.pixelFormat().id())
	{
		default:
			bug_unreachable("format id == %d", attrs.pixelFormat().id());
			return false;
		case PIXEL_RGB565: return
			eglConfigInt(dpy, *configOpt, EGL_BUFFER_SIZE) == 16 &&
			eglConfigInt(dpy, *configOpt, EGL_RED_SIZE) == 5;
		case PIXEL_RGBA8888: return
			eglConfigInt(dpy, *configOpt, EGL_BUFFER_SIZE) == 32 &&
			eglConfigInt(dpy, *configOpt, EGL_RED_SIZE) == 8 &&
			eglConfigInt(dpy, *configOpt, EGL_ALPHA_SIZE) == 8;
	}
}

}
