#pragma once

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

#include <imagine/config/defs.hh>
#include <imagine/base/BaseWindow.hh>
#include <imagine/base/iphone/config.h>
#include <imagine/util/rectangle2.h>
#import <CoreGraphics/CGBase.h>
#include <compare>

#ifdef __OBJC__
#import <UIKit/UIKit.h>
#endif

namespace Base
{

struct NativeWindowFormat {};
using NativeWindow = void*;

#ifdef CONFIG_BASE_IOS_RETINA_SCALE
extern uint32_t screenPointScale;
#else
static constexpr uint32_t screenPointScale = 1;
#endif

class IOSWindow : public BaseWindow
{
public:
	void *uiWin_{}; // UIWindow in ObjC
	IG::WindowRect contentRect{}; // active window content
	#ifdef CONFIG_BASE_IOS_RETINA_SCALE
	CGFloat pointScale{1.};
	#else
	static constexpr CGFloat pointScale{1.};
	#endif

	using BaseWindow::BaseWindow;
	~IOSWindow();
	#ifdef __OBJC__
	void updateContentRect(int width, int height, uint32_t softOrientation);
	UIWindow *uiWin() const { return (__bridge UIWindow*)uiWin_; }
	UIApplication *uiApp() const;
	#endif

	void updateWindowSizeAndContentRect(int width, int height);
	bool isDeviceWindow() const;

	explicit operator bool() const
	{
		return uiWin_;
	}
};

using WindowImpl = IOSWindow;

}
