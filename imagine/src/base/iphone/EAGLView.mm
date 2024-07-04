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
#include <imagine/input/Event.hh>
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

namespace IG
{

EAGLViewMakeRenderbufferDelegate makeRenderbuffer{};
EAGLViewDeleteRenderbufferDelegate deleteRenderbuffer{};

}

using namespace IG;

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

+ (::Class)layerClass
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
	assumeExpr(deleteRenderbuffer);
	deleteRenderbuffer(_colorRenderbuffer, _depthRenderbuffer);
	_colorRenderbuffer = 0;
	_depthRenderbuffer = 0;
}

- (void)dealloc
{
	[self deleteDrawable];
}

- (void)willMoveToWindow:(UIWindow *)newWindow
{
	if(newWindow)
	{
		auto scale = hasAtLeastIOS8() ? [newWindow.screen nativeScale] : [newWindow.screen scale];
		logMsg("view %p moving to window %p with scale %f", self, newWindow, (double)scale);
		self.contentScaleFactor = scale;
	}
	else
	{
		logMsg("view %p removed from window", self);
	}
}

- (void)layoutSubviews
{
	logMsg("in layoutSubviews");
	[self deleteDrawable];
	assumeExpr(makeRenderbuffer);
	auto size = makeRenderbuffer((__bridge void*)self.layer, _colorRenderbuffer, _depthRenderbuffer);
	auto &win = *windowForUIWindow({[UIApplication sharedApplication]}, self.window);
	win.updateWindowSizeAndContentRect(size.x, size.y);
	win.postDraw();
	//logMsg("exiting layoutSubviews");
}

static void dispatchTouches(NSSet *touches, EAGLView *view, Input::Action action)
{
	auto &win = *ApplicationContext{[UIApplication sharedApplication]}.deviceWindow();
	for(UITouch* touch in touches)
	{
		CGPoint pos = [touch locationInView:view];
		pos.x *= win.pointScale;
		pos.y *= win.pointScale;
		auto time = fromSeconds<SteadyClockTime>([touch timestamp]);
		auto transPos = win.transformInputPos({float(pos.x), float(pos.y)});
		win.dispatchInputEvent(Input::MotionEvent{Input::Map::POINTER, Input::Pointer::LBUTTON, 1, action,
			transPos.x, transPos.y, (__bridge void*)touch, Input::Source::TOUCHSCREEN, SteadyClockTimePoint{time}, nullptr});
	}
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	dispatchTouches(touches, self, Input::Action::PUSHED);
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	dispatchTouches(touches, self, Input::Action::MOVED);
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	dispatchTouches(touches, self, Input::Action::RELEASED);
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
	dispatchTouches(touches, self, Input::Action::CANCELED);
}

@end
