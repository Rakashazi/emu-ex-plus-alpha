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
#include <imagine/util/rectangle2.h>
#include <imagine/util/container/ArrayList.hh>
#include <imagine/util/DelegateFunc.hh>
#ifdef CONFIG_INPUT
#include <imagine/input/Input.hh>
#endif

#if defined CONFIG_BASE_X11 && !defined CONFIG_MACHINE_PANDORA
#define CONFIG_BASE_MULTI_WINDOW
#endif

#if defined CONFIG_BASE_IOS && defined __ARM_ARCH_6K__
#define CONFIG_GFX_SOFT_ORIENTATION 1
#elif !defined __ANDROID__ && !defined CONFIG_BASE_IOS
#define CONFIG_GFX_SOFT_ORIENTATION 1
#endif

#if defined CONFIG_BASE_X11
#include <imagine/base/x11/XWindow.hh>
#elif defined CONFIG_BASE_ANDROID
#include <imagine/base/android/AndroidWindow.hh>
#elif defined CONFIG_BASE_IOS
#include <imagine/base/iphone/IOSWindow.hh>
#elif defined CONFIG_BASE_MACOSX
#include <imagine/base/osx/CocoaWindow.hh>
#else
namespace Base
{

class EmptyWindow
{
public:
	void postDraw() {}

	bool operator ==(EmptyWindow const &rhs) const
	{
		return true;
	}

	operator bool() const
	{
		return true;
	}
};

using WindowImpl = EmptyWindow;

}
#endif

namespace Base
{
using namespace IG;

class Screen
{
public:
  static const uint REFRESH_RATE_DEFAULT = 0;
	using OnFrameDelegate = DelegateFunc<void (Screen &screen, FrameTimeBase frameTime)>;
	StaticArrayList<OnFrameDelegate, 4> onFrameDelegate;
	bool framePosted = false;
	bool inFrameHandler = false;

	constexpr Screen() {}
	void postFrame();
	void unpostFrame();
	bool frameIsPosted();
	void addOnFrameDelegate(OnFrameDelegate del);
	bool removeOnFrameDelegate(OnFrameDelegate del);
	bool containsOnFrameDelegate(OnFrameDelegate del);
	void clearOnFrameDelegates();
	void runOnFrameDelegates(FrameTimeBase frameTime);
  #if defined CONFIG_BASE_ANDROID || defined CONFIG_BASE_IOS
  FrameTimeBase lastPostedFrameTime();
  static bool supportsFrameTime();
  #else
  FrameTimeBase lastPostedFrameTime() { return 0; }
  static bool supportsFrameTime() { return false; }
  #endif
  uint refreshRate();
  #ifdef CONFIG_BASE_X11
  void setRefreshRate(uint rate);
  #else
  void setRefreshRate(uint rate) {}
  #endif
};

// orientation
static constexpr uint VIEW_ROTATE_0 = bit(0), VIEW_ROTATE_90 = bit(1), VIEW_ROTATE_180 = bit(2), VIEW_ROTATE_270 = bit(3);
static constexpr uint VIEW_ROTATE_AUTO = bit(5);

static const char *orientationToStr(uint o)
{
	using namespace Base;
	switch(o)
	{
		case VIEW_ROTATE_AUTO: return "Auto";
		case VIEW_ROTATE_0: return "0";
		case VIEW_ROTATE_90: return "90";
		case VIEW_ROTATE_180: return "180";
		case VIEW_ROTATE_270: return "270";
		case VIEW_ROTATE_0 | VIEW_ROTATE_90 | VIEW_ROTATE_270: return "0/90/270";
		case VIEW_ROTATE_0 | VIEW_ROTATE_90 | VIEW_ROTATE_180 | VIEW_ROTATE_270: return "0/90/180/270";
		case VIEW_ROTATE_90 | VIEW_ROTATE_270: return "90/270";
		default: bug_branch("%d", o); return 0;
	}
}

#ifdef CONFIG_GFX
static Gfx::GC orientationToGC(uint o)
{
	switch(o)
	{
		case VIEW_ROTATE_0: return Gfx::angleFromDegree(0.);
		case VIEW_ROTATE_90: return Gfx::angleFromDegree(-90.);
		case VIEW_ROTATE_180: return Gfx::angleFromDegree(-180.);
		case VIEW_ROTATE_270: return Gfx::angleFromDegree(90.);
		default: bug_branch("%d", o); return 0.;
	}
}
#endif

static bool orientationIsSideways(uint rotateView)
{
	return rotateView == VIEW_ROTATE_90 || rotateView == VIEW_ROTATE_270;
}

class Window : public WindowImpl
{
private:
	int w = 0, h = 0; // size of full window surface
	float wMM = 0, hMM = 0; // size in millimeter
	float mmToPixelXScaler = 0, mmToPixelYScaler = 0;
	#ifdef __ANDROID__
	float wSMM = 0, hSMM = 0; // size in millimeter scaled by OS
	float smmToPixelXScaler = 0, smmToPixelYScaler = 0;
	#endif
	bool drawPosted = false;

public:
	bool resizePosted = false;
	bool needsSwap = false;
	#ifdef CONFIG_GFX_SOFT_ORIENTATION
	uint rotateView = VIEW_ROTATE_0;
	uint preferedOrientation = VIEW_ROTATE_0;
	uint validOrientations = Base::VIEW_ROTATE_0 | Base::VIEW_ROTATE_90 | Base::VIEW_ROTATE_180 | Base::VIEW_ROTATE_270;
	#else
	static constexpr uint rotateView = VIEW_ROTATE_0;
	static constexpr uint preferedOrientation = VIEW_ROTATE_0;
	#endif

	constexpr Window() {}

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
	bool shouldAnimateContentBoundsChange() const;

	uint setValidOrientations(uint oMask, bool preferAnimated);
	uint setValidOrientations(uint oMask)
	{
		return setValidOrientations(oMask, true);
	}

	#if defined CONFIG_BASE_X11 || defined CONFIG_BASE_ANDROID || defined CONFIG_BASE_IOS
	static void setPixelBestColorHint(bool best);
	static bool pixelBestColorHintDefault();
	#else
	static void setPixelBestColorHint(bool best) {}
	static bool pixelBestColorHintDefault() { return true; }
	#endif

	// drag & drop
	#if defined CONFIG_BASE_X11
	void setAcceptDnd(bool on);
	#else
	static void setAcceptDnd(bool on) {}
	#endif

	// display swap interval
	#if defined CONFIG_BASE_X11 || defined CONFIG_BASE_IOS || defined CONFIG_BASE_MACOSX
	static void setVideoInterval(uint interval);
	#else
	static void setVideoInterval(uint interval) {}
	#endif

	// window management
	#if defined CONFIG_BASE_X11 || defined CONFIG_BASE_WIN32 || defined CONFIG_BASE_MACOSX
	void setTitle(const char *name);
	#else
	static void setTitle(const char *name) {}
	#endif

	CallResult init(IG::Point2D<int> pos, IG::Point2D<int> size);
	CallResult init(IG::Point2D<int> pos, IG::Point2D<int> size, bool useBestColorFormat);
	void deinit();
	void show();

	// DPI override
	#if defined CONFIG_BASE_X11 || defined CONFIG_BASE_ANDROID
	#define CONFIG_SUPPORTS_DPI_OVERRIDE
	void setDPI(float dpi);
	#else
	static void setDPI(float dpi) {}
	#endif

	void setNeedsDraw(bool needsDraw);
	bool needsDraw();
	void postDraw();
	void unpostDraw();
	void postResize(bool redraw = true);
	void dispatchResize();
	void draw(FrameTimeBase frameTime);
	void setAsDrawTarget();
	void swapBuffers();

	bool updateSize(IG::Point2D<int> surfaceSize);
	bool updatePhysicalSize(IG::Point2D<float> surfaceSizeMM);
	bool updatePhysicalSize(IG::Point2D<float> surfaceSizeMM, IG::Point2D<float> surfaceSizeSMM);
	bool updatePhysicalSizeWithCurrentSize();

	#ifdef CONFIG_GFX_SOFT_ORIENTATION
	uint setOrientation(uint o, bool preferAnimated);
	void setAutoOrientation(bool on);
	void setSystemOrientation(uint o);
	#endif

private:
	IG::Point2D<float> pixelSizeAsMM(IG::Point2D<int> size);
	IG::Point2D<float> pixelSizeAsSMM(IG::Point2D<int> size);
};

Window &mainWindow();
Screen &mainScreen();

// App Callbacks

// Called when app window enters/exits focus
void onFocusChange(Base::Window &win, uint in);

// Called when a file is dropped into into the app's window
// if app enables setAcceptDnd()
void onDragDrop(Base::Window &win, const char *filename);

// Called on app window creation, after the graphics context is initialized
[[gnu::cold]] CallResult onWindowInit(Base::Window &win);

// Called before the window receives onDraw() and
// it's being drawn for the first time, or a different window
// was previously in onDraw(). Gfx settings like the viewport and
// projection matrix should be set here.
void onSetAsDrawTarget(Base::Window &win);

void onDraw(Base::Window &win, FrameTimeBase frameTime);

void onViewChange(Base::Window &win);

#ifdef CONFIG_INPUT
// Called to process an event from an input device
void onInputEvent(Base::Window &win, const Input::Event &event);
#endif

}
