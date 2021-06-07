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
#import <imagine/base/iphone/EAGLView.hh>
#import "MainApp.hh"
#import <OpenGLES/EAGLDrawable.h>
#import <QuartzCore/CAEAGLLayer.h>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/time/Time.hh>
#include <imagine/logger/logger.h>
#include <imagine/input/Input.hh>
#include <imagine/base/GLContext.hh>
#include <imagine/util/algorithm.h>
#include "ios.hh"
#if !defined __ARM_ARCH_6K__
#import <OpenGLES/ES2/gl.h>
#else
#import <OpenGLES/ES1/glext.h>
#define glFramebufferRenderbuffer glFramebufferRenderbufferOES
#define glCheckFramebufferStatus glCheckFramebufferStatusOES
#endif
#include "../../gfx/opengl/utils.hh"

namespace Input
{

static struct TouchState
{
	constexpr TouchState() {}
	UITouch *touch = nil;
} m[Config::Input::MAX_POINTERS];
static uint32_t numCursors = Config::Input::MAX_POINTERS;

}

namespace Base
{

EAGLViewMakeRenderbufferDelegate makeRenderbuffer{};
EAGLViewDeleteRenderbufferDelegate deleteRenderbuffer{};

}

static void bindGLRenderbuffer(GLuint colorRenderbuffer, GLuint depthRenderbuffer)
{
	assert([EAGLContext currentContext]);
	glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
	runGLChecked([&]()
	{
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRenderbuffer);
	}, "glFramebufferRenderbuffer(colorRenderbuffer)");
	runGLChecked([&]()
	{
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
	}, "glFramebufferRenderbuffer(depthRenderbuffer)");
	if(Config::DEBUG_BUILD && glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		logErr("failed to make complete framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
	}
}

@implementation EAGLView
{
	GLuint _colorRenderbuffer;
	GLuint _depthRenderbuffer;
}

+ (Class)layerClass
{
	return [CAEAGLLayer class];
}

- (id)initWithFrame:(CGRect)frame
{
	logMsg("entered initWithFrame");
	self = [super initWithFrame:frame];
	if(self)
	{
		auto eaglLayer = (CAEAGLLayer*)self.layer;
		eaglLayer.opaque = YES;
	}
	else
	{
		logErr("error calling super initWithFrame");
	}
	logMsg("exiting initWithFrame");
	return self;
}
	
- (void)setDrawableColorFormat:(NSString * const)format
{
	auto eaglLayer = (CAEAGLLayer*)self.layer;
	if(format == kEAGLColorFormatRGB565)
	{
		logMsg("using RGB565 surface");
		eaglLayer.drawableProperties = @{ kEAGLDrawablePropertyColorFormat : kEAGLColorFormatRGB565 };
	}
	else
	{
		logMsg("using RGBA8 surface");
		eaglLayer.drawableProperties = @{};
	}
}

- (void)bindDrawable
{
	if(!_colorRenderbuffer)
	{
		logErr("trying to bind view without renderbuffer");
		return;
	}
	bindGLRenderbuffer(_colorRenderbuffer, _depthRenderbuffer);
}

- (void)deleteDrawable
{
	if(!_colorRenderbuffer)
	{
		return; // already deinit
	}
	logMsg("deleting layer renderbuffer: %u", _colorRenderbuffer);
	assumeExpr(Base::deleteRenderbuffer);
	Base::deleteRenderbuffer(_colorRenderbuffer, _depthRenderbuffer);
	_colorRenderbuffer = 0;
	_depthRenderbuffer = 0;
}

- (void)dealloc
{
	[self deleteDrawable];
}

#ifdef CONFIG_BASE_IOS_RETINA_SCALE
- (void)willMoveToWindow:(UIWindow *)newWindow
{
	if(newWindow)
	{
		auto scale = Base::hasAtLeastIOS8() ? [newWindow.screen nativeScale] : [newWindow.screen scale];
		logMsg("view %p moving to window %p with scale %f", self, newWindow, (double)scale);
		self.contentScaleFactor = scale;
	}
	else
	{
		logMsg("view %p removed from window", self);
	}
}
#endif

- (void)layoutSubviews
{
	logMsg("in layoutSubviews");
	[self deleteDrawable];
	assumeExpr(Base::makeRenderbuffer);
	auto size = Base::makeRenderbuffer((__bridge void*)self.layer, _colorRenderbuffer, _depthRenderbuffer);
	auto &win = *Base::windowForUIWindow({[UIApplication sharedApplication]}, self.window);
	win.updateWindowSizeAndContentRect(size.x, size.y);
	win.postDraw();
	//logMsg("exiting layoutSubviews");
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	using namespace Base;
	using namespace Input;
	auto &win = *Base::ApplicationContext{[UIApplication sharedApplication]}.deviceWindow();
	for(UITouch* touch in touches)
	{
		iterateTimes(std::size(m), i) // find a free touch element
		{
			if(Input::m[i].touch == nil)
			{
				auto &p = Input::m[i];
				p.touch = touch;
				CGPoint pos = [touch locationInView:self];
				pos.x *= win.pointScale;
				pos.y *= win.pointScale;
				auto time = IG::FloatSeconds((double)[touch timestamp]);
				auto transPos = win.transformInputPos({(int)pos.x, (int)pos.y});
				win.dispatchInputEvent(Input::Event{i, Map::POINTER, Input::Pointer::LBUTTON, 1, Action::PUSHED, transPos.x, transPos.y, (int)i, Input::Source::TOUCHSCREEN, time, nullptr});
				break;
			}
		}
	}
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	using namespace Base;
	using namespace Input;
	auto &win = *Base::ApplicationContext{[UIApplication sharedApplication]}.deviceWindow();
	for(UITouch* touch in touches)
	{
		iterateTimes(std::size(m), i) // find the touch element
		{
			if(Input::m[i].touch == touch)
			{
				auto &p = Input::m[i];
				CGPoint pos = [touch locationInView:self];
				pos.x *= win.pointScale;
				pos.y *= win.pointScale;
				auto time = IG::FloatSeconds((double)[touch timestamp]);
				auto transPos = win.transformInputPos({(int)pos.x, (int)pos.y});
				win.dispatchInputEvent(Input::Event{i, Map::POINTER, Input::Pointer::LBUTTON, 1, Action::MOVED, transPos.x, transPos.y, (int)i, Input::Source::TOUCHSCREEN, time, nullptr});
				break;
			}
		}
	}
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	using namespace Base;
	using namespace Input;
	auto &win = *Base::ApplicationContext{[UIApplication sharedApplication]}.deviceWindow();
	for(UITouch* touch in touches)
	{
		iterateTimes(std::size(m), i) // find the touch element
		{
			if(Input::m[i].touch == touch)
			{
				auto &p = Input::m[i];
				p.touch = nil;
				CGPoint pos = [touch locationInView:self];
				pos.x *= win.pointScale;
				pos.y *= win.pointScale;
				auto time = IG::FloatSeconds((double)[touch timestamp]);
				auto transPos = win.transformInputPos({(int)pos.x, (int)pos.y});
				win.dispatchInputEvent(Input::Event{i, Map::POINTER, Input::Pointer::LBUTTON, 0, Action::RELEASED, transPos.x, transPos.y, (int)i, Input::Source::TOUCHSCREEN, time, nullptr});
				break;
			}
		}
	}
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
	[self touchesEnded:touches withEvent:event];
}

@end
