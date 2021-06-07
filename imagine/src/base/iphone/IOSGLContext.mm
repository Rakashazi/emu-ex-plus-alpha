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
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Window.hh>
#include <imagine/base/Screen.hh>
#include <imagine/logger/logger.h>

namespace Base
{

// GLDisplay

void GLDisplay::resetCurrentContext() const
{
	[EAGLContext setCurrentContext:nil];
}

// GLDrawable

EAGLViewDrawable::EAGLViewDrawable(void *glView): glView_{glView} {}

GLDisplay GLDrawable::display() const
{
	return {};
}

/*void GLDrawable::freeCaches()
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
}*/

// GLContext

static EAGLRenderingAPI majorVersionToAPI(uint32_t version)
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

IOSGLContext::IOSGLContext(GLContextAttributes attr, NativeGLContext shareContext_, IG::ErrorCode &ec)
{
	assert(attr.openGLESAPI());
	EAGLRenderingAPI api = majorVersionToAPI(attr.majorVersion());
	auto shareContext = (__bridge EAGLContext*)shareContext_;
	EAGLSharegroup *sharegroup = [shareContext sharegroup];
	logMsg("making context with version: %d.%d sharegroup:%p", attr.majorVersion(), attr.minorVersion(), sharegroup);
	EAGLContext *newContext = [[EAGLContext alloc] initWithAPI:api sharegroup:sharegroup];
	if(!newContext)
	{
		logErr("error creating context");
		ec = {EINVAL};
		return;
	}
	context_.reset((NativeGLContext)CFBridgingRetain(newContext));
}

GLDisplay GLContext::display() const
{
	return {};
}

void GLContext::setCurrentContext(NativeGLDrawable drawable) const
{
	auto success = [EAGLContext setCurrentContext:context()];
	assert(success);
	setCurrentDrawable(drawable);
}

void GLContext::setCurrentDrawable(NativeGLDrawable drawable) const
{
	if(!drawable)
		return;
	logMsg("setting view:%p current", drawable);
	auto glView = (__bridge EAGLView*)drawable;
	[glView bindDrawable];
}

void GLContext::present(NativeGLDrawable) const
{
	[context() presentRenderbuffer:GL_RENDERBUFFER];
}

// GLManager

GLManager::GLManager(Base::NativeDisplayConnection ctx, GL::API api)
{
	if(!bindAPI(api))
	{
		logErr("error binding requested API");
	}
}

GLContext GLManager::makeContext(GLContextAttributes attr, GLBufferConfig, NativeGLContext shareContext, IG::ErrorCode &ec)
{
	return GLContext{attr, shareContext, ec};
}

GLDisplay GLManager::getDefaultDisplay(NativeDisplayConnection) const
{
	return {};
}

void GLManager::logInfo() const {}

bool GLManager::bindAPI(GL::API api)
{
	return api == GL::API::OPENGL_ES;
}

GLDrawable GLManager::makeDrawable(Window &win, GLDrawableAttributes config, IG::ErrorCode &) const
{
	CGRect rect = win.screen()->uiScreen().bounds;
	// Create the OpenGL ES view and add it to the Window
	auto glView = [[EAGLView alloc] initWithFrame:rect];
	if(config.bufferConfig().useRGB565)
	{
		[glView setDrawableColorFormat:kEAGLColorFormatRGB565];
	}
	if(*win.screen() == win.appContext().mainScreen())
	{
		glView.multipleTouchEnabled = YES;
	}
	win.uiWin().rootViewController.view = glView;
	return {(NativeGLDrawable)CFBridgingRetain(glView)};
}

bool GLManager::hasCurrentDrawable(NativeGLDrawable drawable)
{
	auto glView = (__bridge EAGLView*)drawable;
	GLint renderBuffer = 0;
	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &renderBuffer);
	return glView.colorRenderbuffer == (GLuint)renderBuffer;
}

bool GLManager::hasCurrentDrawable()
{
	GLint renderBuffer = 0;
	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &renderBuffer);
	return renderBuffer;
}

GLDisplay GLManager::display() const { return {}; }

std::optional<GLBufferConfig> GLManager::makeBufferConfig(Base::ApplicationContext, GLBufferConfigAttributes attr, GL::API, unsigned) const
{
	GLBufferConfig conf;
	if(attr.pixelFormat() == PIXEL_RGB565)
	{
		conf.useRGB565 = true;
	}
	return conf;
}

NativeGLContext GLManager::currentContext()
{
	return (__bridge void*)[EAGLContext currentContext];
}

void *GLManager::procAddress(const char *funcName)
{
	return dlsym(RTLD_DEFAULT, funcName);
}

bool GLManager::hasBufferConfig(GLBufferConfigAttributes attrs) const
{
	switch(attrs.pixelFormat().id())
	{
		default:
			bug_unreachable("format id == %d", attrs.pixelFormat().id());
			return false;
		case PIXEL_NONE:
		case PIXEL_RGB565:
		case PIXEL_RGBA8888: return true;
	}
}

bool GLManager::hasDrawableConfig(GLBufferConfigAttributes attrs, GLColorSpace colorSpace) const
{
	if(colorSpace != GLColorSpace::LINEAR)
		return false;
	return hasBufferConfig(attrs);
}

bool GLManager::hasNoErrorContextAttribute() const
{
	return false;
}

bool GLManager::hasNoConfigContext() const
{
	return true;
}

bool GLManager::hasSrgbColorSpace() const
{
	return false;
}

Base::NativeWindowFormat GLManager::nativeWindowFormat(Base::ApplicationContext, GLBufferConfig) const
{
	return {};
}

bool GLBufferConfig::maySupportGLES(GLDisplay, unsigned majorVersion) const
{
	return majorVersion >= 1 && majorVersion <= 3;
}

}
