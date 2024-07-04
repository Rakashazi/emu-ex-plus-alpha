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
#include <imagine/util/rectangle2.h>
#import <CoreGraphics/CGBase.h>

#ifdef __OBJC__
#import <UIKit/UIKit.h>
#endif

namespace IG
{

struct NativeWindowFormat {};
using NativeWindow = void*;

extern uint32_t screenPointScale;

class IOSWindow : public BaseWindow
{
public:
	void *uiWin_{}; // UIWindow in ObjC
	WRect contentRect{}; // active window content
	CGFloat pointScale{1.};

	using BaseWindow::BaseWindow;
	~IOSWindow();
	#ifdef __OBJC__
	void updateContentRect(int width, int height, Rotation softOrientation);
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
