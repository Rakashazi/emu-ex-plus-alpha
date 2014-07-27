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

static const int USE_DEPTH_BUFFER = 0;

namespace Input
{

static struct TouchState
{
	constexpr TouchState() {}
	UITouch *touch = nil;
	DragPointer dragState;
} m[maxCursors];
uint numCursors = maxCursors;

DragPointer *dragState(int p)
{
	return &m[p].dragState;
}

}

@implementation EAGLView

#ifndef CONFIG_BASE_IOS_GLKIT
// Implement this to override the default layer class (which is [CALayer class]).
// We do this so that our view will be backed by a layer that is capable of OpenGL ES rendering.
+ (Class)layerClass
{
	return [CAEAGLLayer class];
}

//#ifdef CONFIG_BASE_IPHONE_NIB
//// Init from NIB
//- (id)initWithCoder:(NSCoder*)coder
//{
//	if ((self = [super initWithCoder:coder]))
//	{
//		self = [self initGLES];
//	}
//	return self;
//}
//#endif

// Init from code
- (id)initWithFrame:(CGRect)frame context:(EAGLContext *)context
{
	logMsg("entered initWithFrame");
	self = [super initWithFrame:frame];
	if(self)
	{
		auto eaglLayer = (CAEAGLLayer*)self.layer;
		eaglLayer.opaque = YES;
		_context = context;
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
	if(!viewFramebuffer)
	{
		logMsg("creating OpenGL framebuffers");
		glGenFramebuffersOES(1, &viewFramebuffer);
		glGenRenderbuffersOES(1, &viewRenderbuffer);
	
		glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFramebuffer);
		glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbuffer);
		[_context renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:(CAEAGLLayer*)self.layer];
		glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, viewRenderbuffer);
	
		glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &backingWidth);
		glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &backingHeight);
	
		if(USE_DEPTH_BUFFER)
		{
			glGenRenderbuffersOES(1, &depthRenderbuffer);
			glBindRenderbufferOES(GL_RENDERBUFFER_OES, depthRenderbuffer);
			glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT16_OES, backingWidth, backingHeight);
			glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, depthRenderbuffer);
		}
	
		#ifndef NDEBUG
		if(glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES)
		{
			bug_exit("failed to make complete framebuffer object %x", glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES));
		}
		#endif
		
		glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbuffer);
	}
	else
	{
		glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFramebuffer);
		glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbuffer);
	}
}

- (void)deleteDrawable
{
	if(!viewFramebuffer)
	{
		return; // already deinit
	}
	logMsg("deleting OpenGL framebuffers");
	glDeleteFramebuffersOES(1, &viewFramebuffer);
	viewFramebuffer = 0;
	glDeleteRenderbuffersOES(1, &viewRenderbuffer);
	viewRenderbuffer = 0;

	if(depthRenderbuffer)
	{
		glDeleteRenderbuffersOES(1, &depthRenderbuffer);
		depthRenderbuffer = 0;
	}
}

- (void)dealloc
{
	[self deleteDrawable];
}

#else

- (void)bindDrawable
{
	[super bindDrawable];
	if(!viewRenderbuffer)
	{
		glGetIntegerv(GL_RENDERBUFFER_BINDING, (GLint*)&viewRenderbuffer);
		logMsg("got renderbuffer: %d", viewRenderbuffer);
	}
	glBindRenderbuffer(GL_RENDERBUFFER, viewRenderbuffer);
}

- (void)deleteDrawable
{
	[super deleteDrawable];
	viewRenderbuffer = 0;
}

#endif

#ifdef CONFIG_BASE_IOS_RETINA_SCALE
- (void)willMoveToWindow:(UIWindow *)newWindow
{
	if(newWindow)
	{
		logMsg("view %p moving to window %p with scale %f", self, newWindow, [newWindow.screen scale]);
		self.contentScaleFactor = [newWindow.screen scale];
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
	[super layoutSubviews];
	using namespace Base;
	auto &win = *Base::windowForUIWindow(self.window);
	assert(GLContext::current());
	if(GLContext::drawable() == &win)
	{
		GLContext::setDrawable(nullptr);
	}
	GLContext::setDrawable(&win); // rebind to update internal height/width
	#ifdef CONFIG_BASE_IOS_GLKIT
	glGetIntegerv(GL_RENDERBUFFER_BINDING, (GLint*)&viewRenderbuffer);
	updateWindowSizeAndContentRect(win, [self drawableWidth], [self drawableHeight], sharedApp);
	#else
	auto frameSize = self.frame.size;
	updateWindowSizeAndContentRect(win, frameSize.width, frameSize.height, sharedApp);
	#endif
	win.postDraw();
	//logMsg("exiting layoutSubviews");
}

#ifdef CONFIG_INPUT

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	using namespace Base;
	using namespace Input;
	auto &win = *Base::deviceWindow();
	for(UITouch* touch in touches)
	{
		iterateTimes((uint)Input::maxCursors, i) // find a free touch element
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
				win.onInputEvent(win, Input::Event(i, Event::MAP_POINTER, Input::Pointer::LBUTTON, PUSHED, transPos.x, transPos.y, true, 0, nullptr));
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
		iterateTimes((uint)Input::maxCursors, i) // find the touch element
		{
			if(Input::m[i].touch == touch)
			{
				auto &p = Input::m[i];
				CGPoint pos = [touch locationInView:self];
				pos.x *= win.pointScale;
				pos.y *= win.pointScale;
				auto transPos = transformInputPos(win, {(int)pos.x, (int)pos.y});
				p.dragState.pointerEvent(Input::Pointer::LBUTTON, MOVED, transPos);
				win.onInputEvent(win, Input::Event(i, Event::MAP_POINTER, Input::Pointer::LBUTTON, MOVED, transPos.x, transPos.y, true, 0, nullptr));
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
		iterateTimes((uint)Input::maxCursors, i) // find the touch element
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
				win.onInputEvent(win, Input::Event(i, Event::MAP_POINTER, Input::Pointer::LBUTTON, RELEASED, transPos.x, transPos.y, true, 0, nullptr));
				break;
			}
		}
	}
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
	[self touchesEnded:touches withEvent:event];
}

#if defined(CONFIG_BASE_IOS_KEY_INPUT) || defined(CONFIG_INPUT_ICADE)
- (BOOL)canBecomeFirstResponder { return YES; }

- (BOOL)hasText { return NO; }

- (void)insertText:(NSString *)text
{
	#ifdef CONFIG_INPUT_ICADE
	if(Input::iCade.isActive())
		Input::iCade.insertText(text);
	#endif
	//logMsg("got text %s", [text cStringUsingEncoding: NSUTF8StringEncoding]);
}

- (void)deleteBackward {}

	#ifdef CONFIG_INPUT_ICADE
	- (UIView*)inputView
	{
		return Input::iCade.dummyInputView;
	}
	#endif
#endif // defined(CONFIG_BASE_IOS_KEY_INPUT) || defined(CONFIG_INPUT_ICADE)

#endif

@end
