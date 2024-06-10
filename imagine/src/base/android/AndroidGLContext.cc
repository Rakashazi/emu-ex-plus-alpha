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
#include <imagine/logger/logger.h>
#include <imagine/time/Time.hh>
#include <android/native_window.h>

namespace IG
{

GLDisplay GLManager::getDefaultDisplay(NativeDisplayConnection) const
{
	return {eglGetDisplay(EGL_DEFAULT_DISPLAY)};
}

bool GLManager::bindAPI(GL::API api)
{
	return api == GL::API::OpenGLES;
}

std::optional<GLBufferConfig> GLManager::tryBufferConfig(ApplicationContext ctx, const GLBufferRenderConfigAttributes& attrs) const
{
	if(attrs.version.major > 2 && ctx.androidSDK() < 18)
	{
		// need at least Android 4.3 to use ES 3 attributes
		return {};
	}
	auto renderableType = makeRenderableType(GL::API::OpenGLES, attrs.version);
	return chooseConfig(display(), renderableType, attrs);
}

NativeWindowFormat GLManager::nativeWindowFormat(ApplicationContext, GLBufferConfig glConfig) const
{
	EGLint nId;
	auto dpy = display();
	eglGetConfigAttrib(dpy, glConfig, EGL_NATIVE_VISUAL_ID, &nId);
	if(!nId)
	{
		nId = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;
		EGLint redSize;
		eglGetConfigAttrib(dpy, glConfig, EGL_RED_SIZE, &redSize);
		if(redSize < 8)
			nId = AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM;
		//logWarn("config didn't provide a native format id, guessing %d", nId);
	}
	return nId;
}

bool GLManager::hasBufferConfig(GLBufferConfigAttributes attrs) const
{
	switch(attrs.pixelFormat)
	{
		case PixelFmtUnset:
		case PixelFmtRGB565:
		case PixelFmtRGBA8888: return true;
		default: std::unreachable();
	}
}

}
