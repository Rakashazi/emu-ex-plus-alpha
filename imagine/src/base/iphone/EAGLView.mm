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
#import "EAGLView.h"
#import "MainApp.h"
#import <OpenGLES/EAGLDrawable.h>
#import <QuartzCore/CAEAGLLayer.h>
#include <base/Base.hh>
#include <base/common/windowPrivate.hh>
#include <input/Input.hh>
#include <input/DragPointer.hh>

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

#ifdef CONFIG_INPUT_ICADE
#include "ICadeHelper.hh"
namespace Base
{
	extern ICadeHelper iCade;
}
#endif

#ifdef CONFIG_INPUT
#include "input.h"
#endif

namespace Base
{
	extern UIApplication *sharedApp;
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
		#ifdef CONFIG_BASE_IOS_RETINA_SCALE
		if(Base::screenPointScale == 2)
			eaglLayer.contentsScale = 2.0;
		#endif
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
	if(viewFramebuffer)
	{
		return; // already init
	}
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

#endif

- (void)layoutSubviews
{
	logMsg("in layoutSubviews");
	[super layoutSubviews];
	using namespace Base;
	[self bindDrawable]; // rebind to update internal height/width
	#ifdef CONFIG_BASE_IOS_GLKIT
	updateWindowSizeAndContentRect(mainWindow(), [self drawableWidth], [self drawableHeight], sharedApp);
	#else
	auto frameSize = self.frame.size;
	updateWindowSizeAndContentRect(mainWindow(), frameSize.width, frameSize.height, sharedApp);
	#endif
	mainWindow().postResize();
	//logMsg("exiting layoutSubviews");
}

#ifdef CONFIG_INPUT

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	using namespace Base;
	using namespace Input;
	auto &win = Base::mainWindow();
	for(UITouch* touch in touches)
	{
		iterateTimes((uint)Input::maxCursors, i) // find a free touch element
		{
			if(Input::m[i].touch == nil)
			{
				auto &p = Input::m[i];
				p.touch = touch;
				CGPoint startTouchPosition = [touch locationInView:self];
				auto pos = pointerPos(win, startTouchPosition.x * win.pointScale, startTouchPosition.y * win.pointScale);
				//p.s.inWin = 1;
				p.dragState.pointerEvent(Input::Pointer::LBUTTON, PUSHED, pos);
				Base::onInputEvent(win, Input::Event(i, Event::MAP_POINTER, Input::Pointer::LBUTTON, PUSHED, pos.x, pos.y, true, 0, nullptr));
				break;
			}
		}
	}
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	using namespace Base;
	using namespace Input;
	auto &win = Base::mainWindow();
	for(UITouch* touch in touches)
	{
		iterateTimes((uint)Input::maxCursors, i) // find the touch element
		{
			if(Input::m[i].touch == touch)
			{
				auto &p = Input::m[i];
				CGPoint currentTouchPosition = [touch locationInView:self];
				auto pos = pointerPos(win, currentTouchPosition.x * win.pointScale, currentTouchPosition.y * win.pointScale);
				p.dragState.pointerEvent(Input::Pointer::LBUTTON, MOVED, pos);
				Base::onInputEvent(win, Input::Event(i, Event::MAP_POINTER, Input::Pointer::LBUTTON, MOVED, pos.x, pos.y, true, 0, nullptr));
				break;
			}
		}
	}
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	using namespace Base;
	using namespace Input;
	auto &win = Base::mainWindow();
	for(UITouch* touch in touches)
	{
		iterateTimes((uint)Input::maxCursors, i) // find the touch element
		{
			if(Input::m[i].touch == touch)
			{
				auto &p = Input::m[i];
				p.touch = nil;
				//p.s.inWin = 0;
				CGPoint currentTouchPosition = [touch locationInView:self];
				auto pos = pointerPos(win, currentTouchPosition.x * win.pointScale, currentTouchPosition.y * win.pointScale);
				p.dragState.pointerEvent(Input::Pointer::LBUTTON, RELEASED, pos);
				Base::onInputEvent(win, Input::Event(i, Event::MAP_POINTER, Input::Pointer::LBUTTON, RELEASED, pos.x, pos.y, true, 0, nullptr));
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
	if(Base::iCade.isActive())
		Base::iCade.insertText(text);
	#endif
	//logMsg("got text %s", [text cStringUsingEncoding: NSUTF8StringEncoding]);
}

- (void)deleteBackward { }

#ifdef CONFIG_INPUT_ICADE
- (UIView*)inputView
{
	return Base::iCade.dummyInputView;
}
#endif
#endif // defined(CONFIG_BASE_IOS_KEY_INPUT) || defined(CONFIG_INPUT_ICADE)

#endif

@end
