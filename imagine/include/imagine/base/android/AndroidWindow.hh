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
#include <jni.h>
#include <compare>

struct ANativeWindow;

namespace Base
{

using NativeWindowFormat = int32_t;
using NativeWindow = ANativeWindow*;

class AndroidWindow : public BaseWindow
{
public:
	constexpr AndroidWindow() {}
	~AndroidWindow();
	explicit operator bool() const;
	void setNativeWindow(ANativeWindow *nWin);
	int nativePixelFormat();
	void updateContentRect(const IG::WindowRect &rect);
	void setContentRect(const IG::WindowRect &rect, const IG::Point2D<int> &winSize);

protected:
	ANativeWindow *nWin{};
	jobject jDialog{};
	InitDelegate onInit{};
	int32_t pixelFormat = 0;
	IG::WindowRect contentRect; // active window content
	bool initialInit = false;
	static constexpr bool shouldRunOnInitAfterAddingWindow = false;
};

using WindowImpl = AndroidWindow;

}
