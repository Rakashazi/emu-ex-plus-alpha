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

#if defined CONFIG_PACKAGE_X11
#include <imagine/base/x11/XWindow.hh>
#elif defined __ANDROID__
#include <imagine/base/android/AndroidWindow.hh>
#elif defined CONFIG_OS_IOS
#include <imagine/base/iphone/IOSWindow.hh>
#elif defined CONFIG_BASE_MACOSX
#include <imagine/base/osx/CocoaWindow.hh>
#endif

#include <imagine/base/baseDefs.hh>
#include <imagine/input/Event.hh>
#include <imagine/util/Point2D.hh>

namespace IG
{

class ApplicationContext;
class Screen;
class PixelFormat;

struct WindowConfig
{
	Point2D<int> position{-1, -1};
	Point2D<int> size{0, 0};
	Point2D<int> minimumSize{320, 240};
	NativeWindowFormat nativeFormat{};
	Screen *screen_{};
	const char *title{};
	OnWindowEvent onEvent{delegateFuncDefaultInit};
	bool translucent{};

	void setDefaultPosition() { position = {-1, -1}; }
	bool isDefaultPosition() const { return position == Point2D<int>{-1, -1}; }
	void setDefaultSize() { size = {0, 0}; }
	bool isDefaultSize() const { return !size.x || !size.y; }
	void setFormat(PixelFormat);
	void setScreen(Screen &screen) { screen_ = &screen; }
	Screen &screen(ApplicationContext) const;
};

}
