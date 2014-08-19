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

#import <OpenGLES/ES2/gl.h> // for GL_RENDERBUFFER, same values in ES1/ES2
#include <imagine/base/GLContext.hh>
#include <imagine/logger/logger.h>

namespace Base
{

CallResult GLContext::init(const GLContextAttributes &attr, const GLBufferConfig &)
{
	if(attr.majorVersion() == 1)
		context_ = (void*)CFBridgingRetain([[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1]);
	else if(attr.majorVersion() == 2)
		context_ = (void*)CFBridgingRetain([[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2]);
	else
		bug_exit("unsupported OpenGL ES major version: %d", attr.majorVersion());
	assert(context());
	return OK;
}

GLBufferConfig GLContext::makeBufferConfig(const GLContextAttributes &, const GLBufferConfigAttributes &attr)
{
	GLBufferConfig conf;
	if(attr.preferredColorBits() <= 16)
	{
		conf.useRGB565 = true;
	}
	return conf;
}

void GLContext::setCurrent(GLContext c, Window *win)
{
	if(c.context())
	{
		if(win)
		{
			setDrawable(win);
			assert(c.context() == [EAGLContext currentContext]);
		}
		else
		{
			auto success = [EAGLContext setCurrentContext:c.context()];
			assert(success);	
		}
	}
	else
	{
		auto success = [EAGLContext setCurrentContext:nil];
		assert(success);
		assert(!win);
	}
}

void GLContext::setDrawable(Window *win)
{
	if(win)
	{
		[win->glView() bindDrawable];
	}
}

GLContext GLContext::current()
{
	GLContext c;
	c.context_ = (__bridge void*)[EAGLContext currentContext];
	return c;
}

void GLContext::present(Window &win)
{
	[context() presentRenderbuffer:GL_RENDERBUFFER];
}

GLContext::operator bool() const
{
	return context_;
}

void GLContext::deinit()
{
	if(context_)
	{
		CFRelease(context_);
		context_ = nil;
	}
}

}
