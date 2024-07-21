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
#include <imagine/fs/FS.hh>
#include <imagine/util/egl.hh>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/ranges.hh>
#include <imagine/logger/logger.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "xlibutils.h"

namespace IG
{

constexpr SystemLogger log{"X11GL"};

GLDisplay GLManager::getDefaultDisplay(NativeDisplayConnection nativeDpy) const
{
	if constexpr(useEGLPlatformAPI)
	{
		auto dpy = [&]()
		{
			if(FS::exists("/usr/share/glvnd/egl_vendor.d/10_nvidia.json")) // Nvidia EGL library doesn't recognize EGL_PLATFORM_XCB_EXT
			{
				return eglGetPlatformDisplay(EGL_PLATFORM_X11_EXT, EGL_DEFAULT_DISPLAY, nullptr);
			}
			else
			{
				return eglGetPlatformDisplay(EGL_PLATFORM_XCB_EXT, nativeDpy.conn, nullptr);
			}
		}();
		if(Config::DEBUG_BUILD && dpy == EGL_NO_DISPLAY)
			log.error("error:{} getting platform display", GLManager::errorString(eglGetError()));
		return dpy;
	}
	else
	{
		return {eglGetDisplay(EGL_DEFAULT_DISPLAY)};
	}
}

bool GLManager::bindAPI(GL::API api)
{
	if(api == GL::API::OpenGLES)
		return eglBindAPI(EGL_OPENGL_ES_API);
	else
		return eglBindAPI(EGL_OPENGL_API);
}

std::optional<GLBufferConfig> GLManager::tryBufferConfig(ApplicationContext ctx, const GLBufferRenderConfigAttributes& attrs) const
{
	auto renderableType = makeRenderableType(attrs.api, attrs.version);
	if(attrs.bufferAttrs.translucentWindow)
	{
		std::array<EGLConfig, 4> configs;
		auto configCount = chooseConfigs(display(), renderableType, attrs, configs);
		if(!configCount)
		{
			log.error("no usable EGL configs found with renderable type:{}", eglRenderableTypeToStr(renderableType));
			return {};
		}
		// find the config with a visual bits/channel == 8
		for(auto conf : configs | std::views::take(configCount))
		{
			[[maybe_unused]] auto visualId = eglConfigAttrib(display(), conf, EGL_NATIVE_VISUAL_ID);
			auto found = findVisualType(ctx.application().xScreen(), 32, [&](const xcb_visualtype_t& v)
			{
				return v.bits_per_rgb_value == 8;
			});
			if(found)
			{
				if(Config::DEBUG_BUILD)
					printEGLConf(display(), conf);
				return conf;
			}
		}
		log.error("no EGL configs with matching visual bits/channel found");
		return {};
	}
	else
	{
		return chooseConfig(display(), renderableType, attrs);
	}
}

NativeWindowFormat GLManager::nativeWindowFormat(ApplicationContext ctx, GLBufferConfig glConfig) const
{
	if(Config::MACHINE_IS_PANDORA)
		return {};
	// get matching x visual
	EGLint nativeId;
	eglGetConfigAttrib(display(), glConfig, EGL_NATIVE_VISUAL_ID, &nativeId);
	auto viPtr = findVisualType(ctx.application().xScreen(), 0, [&](const xcb_visualtype_t& v)
	{
		return v.visual_id == (uint32_t)nativeId;
	});
	if(!viPtr)
	{
		log.error("unable to find matching X Visual");
		return {};
	}
	return viPtr->visual_id;
}

bool GLManager::hasBufferConfig(GLBufferConfigAttributes attrs) const
{
	if(attrs.pixelFormat == PixelFmtUnset)
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
		case PixelFmtRGB565: return
			eglConfigInt(dpy, *configOpt, EGL_BUFFER_SIZE) == 16 &&
			eglConfigInt(dpy, *configOpt, EGL_RED_SIZE) == 5;
		case PixelFmtRGBA8888: return
			eglConfigInt(dpy, *configOpt, EGL_BUFFER_SIZE) >= 24 &&
			eglConfigInt(dpy, *configOpt, EGL_RED_SIZE) == 8;
		default: std::unreachable();
	}
}

}
