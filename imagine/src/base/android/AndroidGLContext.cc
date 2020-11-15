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
#include <imagine/base/android/android.hh>
#include <imagine/time/Time.hh>
#include <android/native_window.h>

namespace Base
{

// GLDisplay

GLDisplay GLDisplay::getDefault()
{
	return {eglGetDisplay(EGL_DEFAULT_DISPLAY)};
}

bool GLDisplay::bindAPI(API api)
{
	return api == API::OPENGL_ES;
}

// GLContext

std::pair<bool, GLBufferConfig> GLContext::makeBufferConfig(GLDisplay display, GLContextAttributes ctxAttr, GLBufferConfigAttributes attr)
{
	if(ctxAttr.majorVersion() > 2 && Base::androidSDK() < 18)
	{
		// need at least Android 4.3 to use ES 3 attributes
		return {false, {}};
	}
	auto [found, eglConfig] = chooseConfig(display.eglDisplay(), ctxAttr, attr);
	return {found, eglConfig};
}

GLContext::GLContext(GLDisplay display, GLContextAttributes attr, GLBufferConfig config, IG::ErrorCode &ec):
	AndroidGLContext{display.eglDisplay(), attr, config, EGL_NO_CONTEXT, ec}
{}

GLContext::GLContext(GLDisplay display, GLContextAttributes attr, GLBufferConfig config, GLContext shareContext, IG::ErrorCode &ec):
	AndroidGLContext{display.eglDisplay(), attr, config, shareContext.nativeObject(), ec}
{}

void GLContext::deinit(GLDisplay display)
{
	EGLContextBase::deinit(display.eglDisplay());
}

void GLContext::setCurrent(GLDisplay display, GLContext c, GLDrawable win)
{
	setCurrentContext(display.eglDisplay(), c.context, win);
}

void GLContext::present(GLDisplay display, GLDrawable win)
{
	// check if buffer swap blocks even though triple-buffering is used
	auto swapTime = IG::timeFuncDebug([&](){ EGLContextBase::swapBuffers(display.eglDisplay(), win); });
	if(swapBuffersIsAsync() && swapTime > IG::Milliseconds(16))
	{
		logWarn("buffer swap took %lldns", (long long)swapTime.count());
	}
}

void GLContext::present(GLDisplay display, GLDrawable win, GLContext cachedCurrentContext)
{
	present(display, win);
}

bool AndroidGLContext::swapBuffersIsAsync()
{
	return Base::androidSDK() >= 16;
}

Base::NativeWindowFormat EGLBufferConfig::windowFormat(GLDisplay display_)
{
	EGLint nId;
	auto display = display_.eglDisplay();
	eglGetConfigAttrib(display, glConfig, EGL_NATIVE_VISUAL_ID, &nId);
	if(!nId)
	{
		nId = WINDOW_FORMAT_RGBA_8888;
		EGLint alphaSize;
		eglGetConfigAttrib(display, glConfig, EGL_ALPHA_SIZE, &alphaSize);
		if(!alphaSize)
			nId = WINDOW_FORMAT_RGBX_8888;
		EGLint redSize;
		eglGetConfigAttrib(display, glConfig, EGL_RED_SIZE, &redSize);
		if(redSize < 8)
			nId = WINDOW_FORMAT_RGB_565;
		//logWarn("config didn't provide a native format id, guessing %d", nId);
	}
	return nId;
}

}
