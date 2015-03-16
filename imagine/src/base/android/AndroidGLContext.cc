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

#include <GLES/gl.h> // for glFlush()
#include <imagine/base/GLContext.hh>
#include <imagine/logger/logger.h>
#include <imagine/base/android/android.hh>
#include <imagine/time/Time.hh>

namespace Base
{

GLBufferConfig GLContext::makeBufferConfig(GLContextAttributes ctxAttr, GLBufferConfigAttributes attr)
{
	if(ctxAttr.majorVersion() > 2 && Base::androidSDK() < 18)
	{
		// need at least Android 4.3 to use ES 3 attributes
		return GLBufferConfig{};
	}
	auto configResult = chooseConfig(ctxAttr, attr);
	if(std::get<CallResult>(configResult) != OK)
	{
		return GLBufferConfig{};
	}
	return GLBufferConfig{std::get<EGLConfig>(configResult)};
}

EGLDisplay EGLContextBase::getDisplay()
{
	return eglGetDisplay(EGL_DEFAULT_DISPLAY);
}

CallResult GLContext::init(GLContextAttributes attr, GLBufferConfig config)
{
	auto result = EGLContextBase::init(attr, config);
	if(result != OK)
		return result;
	return OK;
}

void GLContext::deinit()
{
	EGLContextBase::deinit();
}

void GLContext::setCurrent(GLContext c, Window *win)
{
	setCurrentContext(c.context, win);
}

void GLContext::present(Window &win)
{
	if(swapBuffersIsAsync())
	{
		// check if buffer swap blocks even though triple-buffering is used
		auto swapTime = IG::timeFuncDebug([&](){ EGLContextBase::swapBuffers(win); }).nSecs();
		if(swapTime > 16000000)
		{
			logWarn("buffer swap took %lldns", (long long)swapTime);
		}
	}
	else
	{
		glFlush();
		win.presented = true;
	}
}

void GLContext::present(Window &win, GLContext cachedCurrentContext)
{
	present(win);
}

void AndroidGLContext::swapPresentedBuffers(Window &win)
{
	if(win.presented)
	{
		assert(!swapBuffersIsAsync()); // shouldn't set presented = true if swap is async
		win.presented = false;
		EGLContextBase::swapBuffers(win);
	}
}

bool AndroidGLContext::swapBuffersIsAsync()
{
	return Base::androidSDK() >= 16;
}

bool GLContext::bindAPI(API api)
{
	return api == OPENGL_ES_API;
}

}
