#pragma once

#include <imagine/engine-globals.h>
#include <imagine/base/BaseWindow.hh>
#include <imagine/util/operators.hh>

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
@interface GLView : NSView {}
@end
#endif

namespace Base
{

class CocoaWindow : public BaseWindow, public NotEquals<CocoaWindow>
{
public:
	void *nsWin_ = nullptr; // NSWindow in ObjC
	void *glView_ = nullptr; // GLView in ObjC

	constexpr CocoaWindow() {}
	#ifdef __OBJC__
	NSWindow *nsWin() { return (__bridge NSWindow*)nsWin_; }
	GLView *glView() { return (__bridge GLView*)glView_; }
	#endif

	bool operator ==(CocoaWindow const &rhs) const
	{
		return nsWin_ == rhs.nsWin_;
	}

	explicit operator bool() const
	{
		return nsWin_;
	}
};

using WindowImpl = CocoaWindow;

struct GLBufferConfig
{
	explicit operator bool() const
	{
		return true;
	}
};


}
