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

static_assert(__has_feature(objc_arc), "This file requires ARC");
#define LOGTAG "EAGL"
#include <dlfcn.h>
#import <OpenGLES/ES2/gl.h> // for GL_RENDERBUFFER, same values in ES1/ES2
#include <imagine/base/GLContext.hh>
#include <imagine/logger/logger.h>
#include "private.hh"

namespace Base
{

// GLDisplay

GLDisplay GLDisplay::makeDefault(std::error_code &ec)
{
	ec = {};
	return {};
}

GLDisplay GLDisplay::getDefault()
{
	return {};
}

GLDisplay::operator bool() const
{
	return true;
}

bool GLDisplay::operator ==(GLDisplay const &rhs) const
{
	return true;
}

void GLDisplay::logInfo() {}

bool GLDisplay::deinit()
{
	return true;
}

GLDrawable GLDisplay::makeDrawable(Window &win, GLBufferConfig config, std::error_code &ec)
{
	CGRect rect = win.screen()->uiScreen().bounds;
	// Create the OpenGL ES view and add it to the Window
	auto glView = [[EAGLView alloc] initWithFrame:rect];
	if(config.useRGB565)
	{
		[glView setDrawableColorFormat:kEAGLColorFormatRGB565];
	}
	if(*win.screen() == mainScreen())
	{
		glView.multipleTouchEnabled = YES;
	}
	//logMsg("setting root view controller");
	auto rootViewCtrl = [[ImagineUIViewController alloc] init];
	#if __IPHONE_OS_VERSION_MIN_REQUIRED < 70000
	rootViewCtrl.wantsFullScreenLayout = YES;
	#endif
	rootViewCtrl.view = glView;
	win.uiWin().rootViewController = rootViewCtrl;
	ec = {};
	return{(void*)CFBridgingRetain(glView)};
}

bool GLDisplay::deleteDrawable(GLDrawable &drawable)
{
	if(drawable.glViewPtr())
	{
		CFRelease(drawable.glViewPtr());
		drawable = {};
	}
	return true;
}

// GLDrawable

void GLDrawable::freeCaches()
{
	if(glView_)
	{
		[glView() deleteDrawable];
	}
}

void GLDrawable::restoreCaches()
{
	if(glView_)
	{
		[glView() layoutSubviews];
	}
}

GLDrawable::operator bool() const
{
	return glView_;
}

bool GLDrawable::operator ==(GLDrawable const &rhs) const
{
	return glView_ == rhs.glView_;
}

bool GLContext::isCurrentDrawable(GLDisplay display, GLDrawable drawable)
{
	GLint renderBuffer = 0;
	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &renderBuffer);
	return drawable.glView().colorRenderbuffer == (GLuint)renderBuffer;
}

// GLContext

static EAGLRenderingAPI majorVersionToAPI(uint version)
{
	switch(version)
	{
		case 1: return kEAGLRenderingAPIOpenGLES1;
		case 2: return kEAGLRenderingAPIOpenGLES2;
		case 3: return kEAGLRenderingAPIOpenGLES3;
		default:
			logErr("unsupported OpenGL ES major version:%u", version);
			return kEAGLRenderingAPIOpenGLES2;
	}
}

GLContext::GLContext(GLDisplay display, GLContextAttributes attr, GLBufferConfig config, GLContext shareContext, std::error_code &ec)
{
	assert(attr.openGLESAPI());
	EAGLRenderingAPI api = majorVersionToAPI(attr.majorVersion());
	EAGLSharegroup *sharegroup = [shareContext.context() sharegroup];
	logMsg("making context with version: %d.%d sharegroup:%p", attr.majorVersion(), attr.minorVersion(), sharegroup);
	EAGLContext *newContext = [[EAGLContext alloc] initWithAPI:api sharegroup:sharegroup];
	if(!newContext)
	{
		logErr("error creating context");
		ec = {EINVAL, std::system_category()};
		return;
	}
	context_ = (void*)CFBridgingRetain(newContext);
	ec = {};
}

GLContext::GLContext(GLDisplay display, GLContextAttributes attr, GLBufferConfig config, std::error_code &ec):
	GLContext{display, attr, config, {}, ec}
{}

GLBufferConfig GLContext::makeBufferConfig(GLDisplay, GLContextAttributes, GLBufferConfigAttributes attr)
{
	GLBufferConfig conf;
	if(attr.pixelFormat() == PIXEL_RGB565)
	{
		conf.useRGB565 = true;
	}
	return conf;
}

void GLContext::setCurrent(GLDisplay, GLContext c, GLDrawable win)
{
	if(c.context())
	{
		auto success = [EAGLContext setCurrentContext:c.context()];
		assert(success);	
		setDrawable({}, win);
	}
	else
	{
		auto success = [EAGLContext setCurrentContext:nil];
		assert(success);
		assert(!win);
	}
}

void GLContext::setDrawable(GLDisplay, GLDrawable win)
{
	if(win)
	{
		logMsg("setting view:%p current", win.glView());
		[win.glView() bindDrawable];
	}
}

void GLContext::setDrawable(GLDisplay, GLDrawable win, GLContext cachedCurrentContext)
{
	setDrawable({}, win);
}

GLContext GLContext::current(GLDisplay)
{
	GLContext c;
	c.context_ = (__bridge void*)[EAGLContext currentContext];
	return c;
}

void GLContext::present(GLDisplay, GLDrawable win)
{
	present({}, win, current({}));
}

void GLContext::present(GLDisplay, GLDrawable win, GLContext cachedCurrentContext)
{
	[cachedCurrentContext.context() presentRenderbuffer:GL_RENDERBUFFER];
}

GLContext::operator bool() const
{
	return context_;
}

bool GLContext::operator ==(GLContext const &rhs) const
{
	return context_ == rhs.context_;
}

void GLContext::deinit(GLDisplay)
{
	if(context_)
	{
		if(context() == [EAGLContext currentContext])
		{
			logMsg("deinit current context:%p", context_);
			[EAGLContext setCurrentContext:nil];
		}
		else
		{
			logMsg("deinit context:%p", context_);
		}
		CFRelease(context_);
		context_ = nil;
	}
}

bool GLContext::bindAPI(API api)
{
	return api == OPENGL_ES_API;
}

void *GLContext::procAddress(const char *funcName)
{
	return dlsym(RTLD_DEFAULT, funcName);
}

NativeGLContext GLContext::nativeObject()
{
	return context_;
}

Base::NativeWindowFormat GLBufferConfig::windowFormat(GLDisplay)
{
	return {};
}

}
