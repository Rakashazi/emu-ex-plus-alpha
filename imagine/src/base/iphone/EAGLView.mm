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
#include <imagine/base/Base.hh>
#include "../common/windowPrivate.hh"
#include <imagine/input/Input.hh>
#include <imagine/input/DragPointer.hh>
#include <imagine/base/GLContext.hh>
#include "ios.hh"
#if !defined __ARM_ARCH_6K__
#import <OpenGLES/ES2/gl.h>
#else
#import <OpenGLES/ES1/glext.h>
#define glGenFramebuffers glGenFramebuffersOES
#define glGenRenderbuffers glGenRenderbuffersOES
#define glDeleteFramebuffers glDeleteFramebuffersOES
#define glDeleteRenderbuffers glDeleteRenderbuffersOES
#define glBindFramebuffer glBindFramebufferOES
#define glBindFramebuffer glBindFramebufferOES
#define glFramebufferRenderbuffer glFramebufferRenderbufferOES
#define glRenderbufferStorage glRenderbufferStorageOES
#define glGetRenderbufferParameteriv glGetRenderbufferParameterivOES
#define glCheckFramebufferStatus glCheckFramebufferStatusOES
#endif

static const int USE_DEPTH_BUFFER = 0;

namespace Input
{

static struct TouchState
{
	constexpr TouchState() {}
	UITouch *touch = nil;
	DragPointer dragState;
} m[Config::Input::MAX_POINTERS];
static uint numCursors = Config::Input::MAX_POINTERS;

DragPointer *dragState(int p)
{
	return &m[p].dragState;
}

}

static GLuint currentGLFramebuffer()
{
	GLint fb;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fb);
	return fb;
}

static IG::Point2D<int> makeLayerGLDrawable(EAGLContext *context,  CAEAGLLayer *layer,
	GLuint &framebuffer, GLuint &colorRenderbuffer, GLuint &depthRenderbuffer)
{
	glGenFramebuffers(1, &framebuffer);
	logMsg("creating layer framebuffer: %u", framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	// make the color renderbuffer
	glGenRenderbuffers(1, &colorRenderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
	[context renderbufferStorage:GL_RENDERBUFFER fromDrawable:layer];
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRenderbuffer);
	GLint backingWidth, backingHeight;
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &backingWidth);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &backingHeight);

	if(USE_DEPTH_BUFFER)
	{
		glGenRenderbuffers(1, &depthRenderbuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, backingWidth, backingHeight);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
	}

	if(Config::DEBUG_BUILD && glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		bug_exit("failed to make complete framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
	}
	return {backingWidth, backingHeight};
}

@implementation EAGLView

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
	if(!framebuffer)
	{
		makeLayerGLDrawable([EAGLContext currentContext], (CAEAGLLayer*)self.layer,
			framebuffer, colorRenderbuffer, depthRenderbuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
	}
	else
	{
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
	}
}

- (void)deleteDrawable
{
	if(!framebuffer)
	{
		return; // already deinit
	}
	logMsg("deleting layer framebuffer: %u", framebuffer);
	glDeleteFramebuffers(1, &framebuffer);
	framebuffer = 0;
	glDeleteRenderbuffers(1, &colorRenderbuffer);
	colorRenderbuffer = 0;
	if(depthRenderbuffer)
	{
		glDeleteRenderbuffers(1, &depthRenderbuffer);
		depthRenderbuffer = 0;
	}
}

- (void)dealloc
{
	assert(Base::GLContext::current());
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
	using namespace Base;
	assert(GLContext::current());
	[self deleteDrawable];
	auto size = makeLayerGLDrawable([EAGLContext currentContext], (CAEAGLLayer*)self.layer,
		framebuffer, colorRenderbuffer, depthRenderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
	auto &win = *Base::windowForUIWindow(self.window);
	onGLDrawableChanged.callCopySafe(&win);
	updateWindowSizeAndContentRect(win, size.x, size.y, sharedApp);
	win.postDraw();
	//logMsg("exiting layoutSubviews");
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	using namespace Base;
	using namespace Input;
	auto &win = *Base::deviceWindow();
	for(UITouch* touch in touches)
	{
		iterateTimes(sizeofArray(m), i) // find a free touch element
		{
			if(Input::m[i].touch == nil)
			{
				auto &p = Input::m[i];
				p.touch = touch;
				CGPoint pos = [touch locationInView:self];
				pos.x *= win.pointScale;
				pos.y *= win.pointScale;
				auto transPos = transformInputPos(win, {(int)pos.x, (int)pos.y});
				p.dragState.pointerEvent(Input::Pointer::LBUTTON, PUSHED, transPos);
				win.dispatchInputEvent(Input::Event{i, Event::MAP_POINTER, Input::Pointer::LBUTTON, PUSHED, transPos.x, transPos.y, true, 0, nullptr});
				break;
			}
		}
	}
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	using namespace Base;
	using namespace Input;
	auto &win = *Base::deviceWindow();
	for(UITouch* touch in touches)
	{
		iterateTimes(sizeofArray(m), i) // find the touch element
		{
			if(Input::m[i].touch == touch)
			{
				auto &p = Input::m[i];
				CGPoint pos = [touch locationInView:self];
				pos.x *= win.pointScale;
				pos.y *= win.pointScale;
				auto transPos = transformInputPos(win, {(int)pos.x, (int)pos.y});
				p.dragState.pointerEvent(Input::Pointer::LBUTTON, MOVED, transPos);
				win.dispatchInputEvent(Input::Event{i, Event::MAP_POINTER, Input::Pointer::LBUTTON, MOVED, transPos.x, transPos.y, true, 0, nullptr});
				break;
			}
		}
	}
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	using namespace Base;
	using namespace Input;
	auto &win = *Base::deviceWindow();
	for(UITouch* touch in touches)
	{
		iterateTimes(sizeofArray(m), i) // find the touch element
		{
			if(Input::m[i].touch == touch)
			{
				auto &p = Input::m[i];
				p.touch = nil;
				CGPoint pos = [touch locationInView:self];
				pos.x *= win.pointScale;
				pos.y *= win.pointScale;
				auto transPos = transformInputPos(win, {(int)pos.x, (int)pos.y});
				p.dragState.pointerEvent(Input::Pointer::LBUTTON, RELEASED, transPos);
				win.dispatchInputEvent(Input::Event{i, Event::MAP_POINTER, Input::Pointer::LBUTTON, RELEASED, transPos.x, transPos.y, true, 0, nullptr});
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
