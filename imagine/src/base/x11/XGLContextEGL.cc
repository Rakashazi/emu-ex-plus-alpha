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
#include <imagine/util/egl.hh>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/ranges.hh>
#include <imagine/logger/logger.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <X11/Xutil.h>

#ifndef EGL_PLATFORM_X11_EXT
#define EGL_PLATFORM_X11_EXT 0x31D5
#endif

namespace IG
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

std::optional<GLBufferConfig> GLManager::makeBufferConfig(ApplicationContext ctx, GLBufferConfigAttributes attr, GL::API api, int majorVersion) const
{
	auto renderableType = makeRenderableType(api, majorVersion);
	if(attr.translucentWindow)
	{
		std::array<EGLConfig, 4> configs;
		auto configCount = chooseConfigs(display(), renderableType, attr, configs);
		if(!configCount)
		{
			logErr("no usable EGL configs found with renderable type:%s", eglRenderableTypeToStr(renderableType));
			return {};
		}
		// find the config with a visual bits/channel == 8
		auto xDpy = static_cast<Display*>(ctx.nativeDisplayConnection());
		for(auto conf : configs | std::views::take(configCount))
		{
			XVisualInfo visualTemplate{};
			visualTemplate.c_class = TrueColor;
			visualTemplate.visualid = eglConfigAttrib(display(), conf, EGL_NATIVE_VISUAL_ID);
			int count;
			auto infoPtr = XGetVisualInfo(xDpy, VisualIDMask | VisualClassMask, &visualTemplate, &count);
			if(!infoPtr)
				continue;
			auto freeInfo = scopeGuard([&](){ XFree(infoPtr); });
			if(infoPtr->bits_per_rgb == 8)
			{
				if(Config::DEBUG_BUILD)
					printEGLConf(display(), conf);
				return conf;
			}
		}
		logErr("no EGL configs with matching visual bits/channel found");
		return {};
	}
	else
	{
		return chooseConfig(display(), renderableType, attr);
	}
}

NativeWindowFormat GLManager::nativeWindowFormat(ApplicationContext ctx, GLBufferConfig glConfig) const
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
	if(attrs.pixelFormat.id == PIXEL_NONE)
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
	switch(attrs.pixelFormat.id)
	{
		case PIXEL_RGB565: return
			eglConfigInt(dpy, *configOpt, EGL_BUFFER_SIZE) == 16 &&
			eglConfigInt(dpy, *configOpt, EGL_RED_SIZE) == 5;
		case PIXEL_RGBA8888: return
			eglConfigInt(dpy, *configOpt, EGL_BUFFER_SIZE) >= 24 &&
			eglConfigInt(dpy, *configOpt, EGL_RED_SIZE) == 8;
		default: bug_unreachable("format id == %d", attrs.pixelFormat.id);
	}
}

}
