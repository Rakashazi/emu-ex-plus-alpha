#pragma once

#include <imagine/config/defs.hh>
#include <imagine/base/BaseWindow.hh>

namespace IG
{

struct NativeWindowFormat {};
using NativeWindow = void*;

class CocoaWindow : public BaseWindow
{
public:
	void *nsWin_ = nullptr; // NSWindow in ObjC

	constexpr CocoaWindow() {}
	#ifdef __OBJC__
	NSWindow *nsWin() { return (__bridge NSWindow*)nsWin_; }
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

}
