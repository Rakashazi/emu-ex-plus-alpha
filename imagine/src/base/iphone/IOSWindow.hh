#pragma once

#include <engine-globals.h>
#include <util/operators.hh>

#ifdef __OBJC__
#import <base/iphone/EAGLView.hh>
#else
struct UIWindow;
struct EAGLView;
#endif

namespace Base
{

class IOSWindow : public NotEquals<IOSWindow>
{
public:
	UIWindow *uiWin = nullptr;
	EAGLView *glView = nullptr;
	IG::Rect2<int> contentRect; // active window content
	#if !defined(__ARM_ARCH_6K__)
	uint pointScale = 1;
	#else
	static constexpr uint pointScale = 1;
	#endif

	constexpr IOSWindow() {}

	bool operator ==(IOSWindow const &rhs) const
	{
		return uiWin == rhs.uiWin;
	}

	operator bool() const
	{
		return uiWin;
	}
};

using WindowImpl = IOSWindow;

}
