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

#if defined CONFIG_BASE_X11
#include <imagine/base/x11/XWindow.hh>
#elif defined CONFIG_BASE_ANDROID
#include <imagine/base/android/AndroidWindow.hh>
#elif defined CONFIG_BASE_IOS
#include <imagine/base/iphone/IOSWindow.hh>
#elif defined CONFIG_BASE_MACOSX
#include <imagine/base/osx/CocoaWindow.hh>
#endif

#include <imagine/base/baseDefs.hh>

namespace IG
{
class PixelFormat;
}

namespace Base
{

using namespace IG;

class ApplicationContext;
class Screen;

class WindowConfig
{
public:
	constexpr WindowConfig() {}

	void setDefaultPosition()
	{
		pos = {-1, -1};
	}

	bool isDefaultPosition() const
	{
		return pos == Point2D<int>{-1, -1};
	}

	void setPosition(Point2D<int> pos)
	{
		this->pos = pos;
	}

	Point2D<int> position() const
	{
		return pos;
	}

	void setDefaultSize()
	{
		size_ = {0, 0};
	}

	bool isDefaultSize() const
	{
		return !size_.x || !size_.y;
	}

	void setSize(Point2D<int> size_)
	{
		this->size_ = size_;
	}

	Point2D<int> size() const
	{
		return size_;
	}

	void setMinimumSize(Point2D<int> minSize)
	{
		this->minSize = minSize;
	}

	Point2D<int> minimumSize() const
	{
		return minSize;
	}

	void setFormat(NativeWindowFormat fmt)
	{
		this->fmt = fmt;
	}

	void setFormat(IG::PixelFormat);

	NativeWindowFormat format() const
	{
		return fmt;
	}

	void setScreen(Screen &screen)
	{
		screen_ = &screen;
	}

	Screen &screen(ApplicationContext) const;

	void setOnSurfaceChange(WindowSurfaceChangeDelegate del)
	{
		onSurfaceChange_ = del;
	}

	WindowSurfaceChangeDelegate onSurfaceChange() const
	{
		return onSurfaceChange_;
	}

	void setOnDraw(WindowDrawDelegate del)
	{
		onDraw_ = del;
	}

	WindowDrawDelegate onDraw() const
	{
		return onDraw_;
	}

	void setOnInputEvent(WindowInputEventDelegate del)
	{
		onInputEvent_ = del;
	}

	WindowInputEventDelegate onInputEvent() const
	{
		return onInputEvent_;
	}

	void setOnFocusChange(WindowFocusChangeDelegate del)
	{
		onFocusChange_ = del;
	}

	WindowFocusChangeDelegate onFocusChange() const
	{
		return onFocusChange_;
	}

	void setOnDragDrop(WindowDragDropDelegate del)
	{
		onDragDrop_ = del;
	}

	WindowDragDropDelegate onDragDrop() const
	{
		return onDragDrop_;
	}

	void setOnDismissRequest(WindowDismissRequestDelegate del)
	{
		onDismissRequest_ = del;
	}

	WindowDismissRequestDelegate onDismissRequest() const
	{
		return onDismissRequest_;
	}

	void setOnDismiss(WindowDismissDelegate del)
	{
		onDismiss_ = del;
	}

	WindowDismissDelegate onDismiss() const
	{
		return onDismiss_;
	}

	void setTitle(const char *str)
	{
		title_ = str;
	}

	const char *title() const
	{
		return title_;
	}

private:
	Point2D<int> pos{-1, -1};
	Point2D<int> size_{0, 0};
	Point2D<int> minSize{320, 240};
	NativeWindowFormat fmt{};
	Screen *screen_{};
	const char *title_{};
	WindowSurfaceChangeDelegate onSurfaceChange_;
	WindowDrawDelegate onDraw_;
	WindowInputEventDelegate onInputEvent_;
	WindowFocusChangeDelegate onFocusChange_;
	WindowDragDropDelegate onDragDrop_;
	WindowDismissRequestDelegate onDismissRequest_;
	WindowDismissDelegate onDismiss_;
};

}
