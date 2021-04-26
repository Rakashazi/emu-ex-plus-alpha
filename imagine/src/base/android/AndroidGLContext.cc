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

namespace Base
{

// GLDisplay

GLDisplay GLDisplay::getDefault(Base::NativeDisplayConnection)
{
	return {eglGetDisplay(EGL_DEFAULT_DISPLAY)};
}

bool GLDisplay::bindAPI(GL::API api)
{
	return api == GL::API::OPENGL_ES;
}

// GLContext

std::optional<GLBufferConfig> GLContext::makeBufferConfig(GLDisplay display, ApplicationContext ctx, GLBufferConfigAttributes attr, GL::API api, unsigned majorVersion)
{
	if(majorVersion > 2 && ctx.androidSDK() < 18)
	{
		// need at least Android 4.3 to use ES 3 attributes
		return {};
	}
	auto renderableType = GLDisplay::makeRenderableType(GL::API::OPENGL_ES, majorVersion);
	return chooseConfig(display, renderableType, attr);
}

GLContext::GLContext(GLDisplay display, GLContextAttributes attr, GLBufferConfig config, IG::ErrorCode &ec):
	EGLContextBase{display, attr, config, EGL_NO_CONTEXT, ec}
{}

GLContext::GLContext(GLDisplay display, GLContextAttributes attr, GLBufferConfig config, GLContext shareContext, IG::ErrorCode &ec):
	EGLContextBase{display, attr, config, shareContext.nativeObject(), ec}
{}

void GLContext::deinit(GLDisplay display)
{
	EGLContextBase::deinit(display);
}

void GLContext::setCurrent(GLDisplay display, GLContext c, GLDrawable win)
{
	setCurrentContext(display, c.context, win);
}

void GLContext::present(GLDisplay display, GLDrawable win)
{
	EGLContextBase::swapBuffers(display, win);
}

void GLContext::present(GLDisplay display, GLDrawable win, GLContext cachedCurrentContext)
{
	present(display, win);
}

Base::NativeWindowFormat EGLBufferConfig::windowFormat(Base::ApplicationContext, GLDisplay display) const
{
	EGLint nId;
	eglGetConfigAttrib(display, glConfig, EGL_NATIVE_VISUAL_ID, &nId);
	if(!nId)
	{
		nId = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;
		EGLint alphaSize;
		eglGetConfigAttrib(display, glConfig, EGL_ALPHA_SIZE, &alphaSize);
		if(!alphaSize)
			nId = AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM;
		EGLint redSize;
		eglGetConfigAttrib(display, glConfig, EGL_RED_SIZE, &redSize);
		if(redSize < 8)
			nId = AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM;
		//logWarn("config didn't provide a native format id, guessing %d", nId);
	}
	return nId;
}

}
