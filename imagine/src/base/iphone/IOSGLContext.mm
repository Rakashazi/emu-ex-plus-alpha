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

namespace IG
{

constexpr SystemLogger log{"EAGL"};

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

static EAGLRenderingAPI majorVersionToAPI(int version)
{
	switch(version)
	{
		case 1: return kEAGLRenderingAPIOpenGLES1;
		case 2: return kEAGLRenderingAPIOpenGLES2;
		case 3: return kEAGLRenderingAPIOpenGLES3;
		default:
			log.error("unsupported OpenGL ES major version:{}", version);
			return kEAGLRenderingAPIOpenGLES2;
	}
}

IOSGLContext::IOSGLContext(GLContextAttributes attr, NativeGLContext shareContext_)
{
	assert(attr.api == GL::API::OpenGLES);
	EAGLRenderingAPI api = majorVersionToAPI(attr.version.major);
	auto shareContext = (__bridge EAGLContext*)shareContext_;
	EAGLSharegroup *sharegroup = [shareContext sharegroup];
	log.info("making context with version:{} sharegroup:{}", attr.version.major, (__bridge void*)sharegroup);
	EAGLContext *newContext = [[EAGLContext alloc] initWithAPI:api sharegroup:sharegroup];
	if(!newContext)
	{
		throw std::runtime_error("Error creating GL context");
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
	log.info("setting view:{} current", drawable);
	auto glView = (__bridge EAGLView*)drawable;
	[glView bindDrawable];
}

void GLContext::present(NativeGLDrawable) const
{
	[context() presentRenderbuffer:GL_RENDERBUFFER];
}

void GLContext::setSwapInterval(int) {}

// GLManager

GLManager::GLManager(NativeDisplayConnection, GL::API api)
{
	if(!bindAPI(api))
	{
		log.error("error binding requested API");
	}
}

GLContext GLManager::makeContext(GLContextAttributes attr, GLBufferConfig, NativeGLContext shareContext)
{
	return GLContext{attr, shareContext};
}

GLDisplay GLManager::getDefaultDisplay(NativeDisplayConnection) const
{
	return {};
}

void GLManager::logInfo() const {}

bool GLManager::bindAPI(GL::API api)
{
	return api == GL::API::OpenGLES;
}

GLDrawable GLManager::makeDrawable(Window &win, GLDrawableAttributes config) const
{
	CGRect rect = win.screen()->uiScreen().bounds;
	// Create the OpenGL ES view and add it to the Window
	auto glView = [[EAGLView alloc] initWithFrame:rect];
	if(config.bufferConfig.useRGB565)
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

std::optional<GLBufferConfig> GLManager::tryBufferConfig(ApplicationContext, const GLBufferRenderConfigAttributes& attrs) const
{
	GLBufferConfig conf;
	if(attrs.bufferAttrs.pixelFormat == PixelFmtRGB565)
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
	switch(attrs.pixelFormat)
	{
		case PixelFmtUnset:
		case PixelFmtRGB565:
		case PixelFmtRGBA8888: return true;
		default: std::unreachable();
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

NativeWindowFormat GLManager::nativeWindowFormat(ApplicationContext, GLBufferConfig) const
{
	return {};
}

bool GLManager::hasPresentationTime() const { return false; }

bool GLBufferConfig::maySupportGLES(GLDisplay, int majorVersion) const
{
	return majorVersion >= 1 && majorVersion <= 3;
}

}
