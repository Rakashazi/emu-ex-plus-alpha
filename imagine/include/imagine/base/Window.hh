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
#include <imagine/gfx/defs.hh>
#include <imagine/base/baseDefs.hh>
#include <imagine/base/Screen.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/util/container/ArrayList.hh>
#include <imagine/util/DelegateFunc.hh>
#include <imagine/util/bits.h>
#include <imagine/input/Input.hh>

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
	GLConfig glConfig_;
	Screen *screen_ = nullptr;

public:
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

	void setGLConfig(GLConfig glConfig_)
	{
		var_selfs(glConfig_);
	}

	GLConfig glConfig() const
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
};

class Window : public WindowImpl
{
public:
	constexpr Window() {}

	CallResult init(const WindowConfig &config);
	void show();
	void dismiss();
	void setAcceptDnd(bool on);
	void setTitle(const char *name);
	void setNeedsDraw(bool needsDraw);
	void setNeedsCustomViewportResize(bool needsResize);
	bool needsDraw();
	void postDraw();
	void unpostDraw();
	void dispatchOnDraw(FrameTimeBase frameTime);
	Screen &screen();
	static void postNeededScreens();
	static uint windows();
	static Window *window(uint idx);

	// Called when the state of the window's drawing surface changes,
	// such as a re-size or if it becomes the current drawing target
	void setOnSurfaceChange(SurfaceChangeDelegate del);
	// Called during a Screen frame callback if the window needs to be drawn
	void setOnDraw(DrawDelegate del);
	// Called to process an event from an input device
	void setOnInputEvent(InputEventDelegate del);
	// Called when app window enters/exits focus
	void setOnFocusChange(FocusChangeDelegate del);
	// Called when a file is dropped into into the app's window
	// if app enables setAcceptDnd()
	void setOnDragDrop(DragDropDelegate del);
	// Called when the user performs an action indicating to
	// to the window manager they wish to dismiss the window
	// (clicking the close button for example),
	// by default it will exit the app
	void setOnDismissRequest(DismissRequestDelegate del);
	// Called when the window is dismissed
	void setOnDismiss(DismissDelegate del);

	int realWidth() const { return orientationIsSideways(rotateView) ? h : w; }
	int realHeight() const { return orientationIsSideways(rotateView) ? w : h; }
	int width() const { return w; }
	int height() const { return h; }
	IG::Point2D<int> realSize() const { return {realWidth(), realHeight()}; }
	IG::Point2D<int> size() const { return {width(), height()}; }

	float widthMM() const
	{
		assert(wMM);
		return wMM;
	}

	float heightMM() const
	{
		assert(hMM);
		return hMM;
	}

	#ifdef __ANDROID__
	float widthSMM() const
	{
		assert(wSMM);
		return wSMM;
	}

	float heightSMM() const
	{
		assert(hSMM);
		return hSMM;
	}
	#endif

	int widthMMInPixels(float mm) const
	{
		return std::round(mm * (mmToPixelXScaler));
	}

	int heightMMInPixels(float mm) const
	{
		return std::round(mm * (mmToPixelYScaler));
	}

	#ifdef __ANDROID__
	int widthSMMInPixels(float mm) const
	{
		return std::round(mm * (smmToPixelXScaler));
	}
	int heightSMMInPixels(float mm) const
	{
		return std::round(mm * (smmToPixelYScaler));
	}
	#else
	int widthSMMInPixels(float mm) const { return widthMMInPixels(mm); }
	int heightSMMInPixels(float mm) const { return heightMMInPixels(mm); }
	#endif

	IG::WindowRect bounds() const
	{
		return {0, 0, width(), height()};
	}

	// content in these bounds isn't blocked by system overlays and receives pointer input
	IG::WindowRect contentBounds() const;

	#ifdef CONFIG_GFX_SOFT_ORIENTATION
	uint setOrientation(uint o, bool preferAnimated);
	void setAutoOrientation(bool on);
	void setSystemOrientation(uint o);
	#endif
	uint setValidOrientations(uint oMask, bool preferAnimated);
	uint setValidOrientations(uint oMask)
	{
		return setValidOrientations(oMask, true);
	}

	bool updateSize(IG::Point2D<int> surfaceSize);
	bool updatePhysicalSize(IG::Point2D<float> surfaceSizeMM);
	bool updatePhysicalSize(IG::Point2D<float> surfaceSizeMM, IG::Point2D<float> surfaceSizeSMM);
	bool updatePhysicalSizeWithCurrentSize();
	bool hasSurface();
	void dispatchInputEvent(const Input::Event &event);
	void dispatchFocusChange(bool in);
	void dispatchDragDrop(const char *filename);
	void dispatchDismissRequest();
	void deinit();

private:
	IG::Point2D<float> pixelSizeAsMM(IG::Point2D<int> size);
	IG::Point2D<float> pixelSizeAsSMM(IG::Point2D<int> size);
	void initDelegates();
	void dispatchSurfaceChange();
};

Window &mainWindow();
Screen &mainScreen();

}
