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

#include <imagine/engine-globals.h>
#include <imagine/base/Screen.hh>
#include <imagine/util/DelegateFunc.hh>

namespace Config
{
#if (defined CONFIG_BASE_X11 && !defined CONFIG_MACHINE_PANDORA) || defined CONFIG_BASE_MULTI_SCREEN
#define CONFIG_BASE_MULTI_WINDOW
static constexpr bool BASE_MULTI_WINDOW = true;
#else
static constexpr bool BASE_MULTI_WINDOW = false;
#endif
}

#if defined CONFIG_BASE_X11
#include <imagine/base/x11/XWindow.hh>
#elif defined CONFIG_BASE_ANDROID
#include <imagine/base/android/AndroidWindow.hh>
#elif defined CONFIG_BASE_IOS
#include <imagine/base/iphone/IOSWindow.hh>
#elif defined CONFIG_BASE_MACOSX
#include <imagine/base/osx/CocoaWindow.hh>
#endif

namespace Base
{

using namespace IG;

class WindowConfig
{
private:
	Point2D<int> pos{-1, -1};
	Point2D<int> size_{0, 0};
	Point2D<int> minSize{320, 240};
	GLBufferConfig glConfig_{};
	Screen *screen_{};
	BaseWindow::SurfaceChangeDelegate onSurfaceChange_;
	BaseWindow::DrawDelegate onDraw_;
	BaseWindow::InputEventDelegate onInputEvent_;
	BaseWindow::FocusChangeDelegate onFocusChange_;
	BaseWindow::DragDropDelegate onDragDrop_;
	BaseWindow::DismissRequestDelegate onDismissRequest_;
	BaseWindow::DismissDelegate onDismiss_;

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
		var_selfs(pos);
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
		var_selfs(size_);
	}

	Point2D<int> size() const
	{
		return size_;
	}

	void setMinimumSize(Point2D<int> minSize)
	{
		var_selfs(minSize);
	}

	Point2D<int> minimumSize() const
	{
		return minSize;
	}

	void setGLConfig(GLBufferConfig glConfig_)
	{
		var_selfs(glConfig_);
	}

	GLBufferConfig glConfig() const
	{
		return glConfig_;
	}

	void setScreen(Screen &screen)
	{
		screen_ = &screen;
	}

	Screen &screen() const
	{
		return screen_ ? *screen_ : *Screen::screen(0);
	}

	void setOnSurfaceChange(BaseWindow::SurfaceChangeDelegate del)
	{
		onSurfaceChange_ = del;
	}

	BaseWindow::SurfaceChangeDelegate onSurfaceChange() const
	{
		return onSurfaceChange_;
	}

	void setOnDraw(BaseWindow::DrawDelegate del)
	{
		onDraw_ = del;
	}

	BaseWindow::DrawDelegate onDraw() const
	{
		return onDraw_;
	}

	void setOnInputEvent(BaseWindow::InputEventDelegate del)
	{
		onInputEvent_ = del;
	}

	BaseWindow::InputEventDelegate onInputEvent() const
	{
		return onInputEvent_;
	}

	void setOnFocusChange(BaseWindow::FocusChangeDelegate del)
	{
		onFocusChange_ = del;
	}

	BaseWindow::FocusChangeDelegate onFocusChange() const
	{
		return onFocusChange_;
	}

	void setOnDragDrop(BaseWindow::DragDropDelegate del)
	{
		onDragDrop_ = del;
	}

	BaseWindow::DragDropDelegate onDragDrop() const
	{
		return onDragDrop_;
	}

	void setOnDismissRequest(BaseWindow::DismissRequestDelegate del)
	{
		onDismissRequest_ = del;
	}

	BaseWindow::DismissRequestDelegate onDismissRequest() const
	{
		return onDismissRequest_;
	}

	void setOnDismiss(BaseWindow::DismissDelegate del)
	{
		onDismiss_ = del;
	}

	BaseWindow::DismissDelegate onDismiss() const
	{
		return onDismiss_;
	}
};

}
