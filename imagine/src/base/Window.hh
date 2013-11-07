#pragma once

#include <engine-globals.h>
#include <config/machine.hh>
#include <gfx/defs.hh>
#include <util/rectangle2.h>

#if defined CONFIG_BASE_X11 && !defined CONFIG_MACHINE_PANDORA
#define CONFIG_BASE_MULTI_WINDOW
#endif

#if defined CONFIG_BASE_X11
#include <base/x11/XWindow.hh>
#elif defined CONFIG_BASE_ANDROID
#include <base/android/AndroidWindow.hh>
#elif defined CONFIG_BASE_IOS
#include <base/iphone/IOSWindow.hh>
#elif defined CONFIG_BASE_MACOSX
#include <base/osx/CocoaWindow.hh>
#else
namespace Base
{

class EmptyWindow
{
public:
	void displayNeedsUpdate() {}

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
		case VIEW_ROTATE_90 | VIEW_ROTATE_270: return "90/270";
		default: bug_branch("%d", o); return 0;
	}
}

class Window : public WindowImpl
{
public:
	IG::Rect2<int> viewRect;
	int w = 0, h = 0; // size of full window surface
	bool drawPosted = false;
	bool resizePosted = false;
	bool needsSwap = false;
	#ifdef CONFIG_GFX_SOFT_ORIENTATION
	uint rotateView = VIEW_ROTATE_0;
	uint preferedOrientation = VIEW_ROTATE_0;
	#else
	static constexpr uint rotateView = VIEW_ROTATE_0;
	static constexpr uint preferedOrientation = VIEW_ROTATE_0;
	#endif
	float mmToPixelXScaler = 0, mmToPixelYScaler = 0;
	#ifdef CONFIG_BASE_ANDROID
	float smmToPixelXScaler = 0, smmToPixelYScaler = 0;
	#endif
	uint viewPixelWidth_ = 0, viewPixelHeight_ = 0;
	GC viewMMWidth_ = 0, viewMMHeight_ = 0,  // in MM
		viewSMMWidth_ = 0, viewSMMHeight_ = 0; // in MM from scaled screen units

	constexpr Window() {}

	uint viewPixelWidth() const { return (rotateView == VIEW_ROTATE_90 || rotateView == VIEW_ROTATE_270) ? viewPixelHeight_ : viewPixelWidth_; }
	uint viewPixelHeight() const { return (rotateView == VIEW_ROTATE_90 || rotateView == VIEW_ROTATE_270) ? viewPixelWidth_ : viewPixelHeight_; }

	// pixel density
	GC viewMMWidth() const { return (rotateView == VIEW_ROTATE_90 || rotateView == VIEW_ROTATE_270) ? viewMMHeight_ : viewMMWidth_; }
	GC viewMMHeight() const { return (rotateView == VIEW_ROTATE_90 || rotateView == VIEW_ROTATE_270) ? viewMMWidth_ : viewMMHeight_; }
	#ifdef CONFIG_BASE_ANDROID
	GC viewSMMWidth() const { return (rotateView == VIEW_ROTATE_90 || rotateView == VIEW_ROTATE_270) ? viewSMMHeight_ : viewSMMWidth_; }
	GC viewSMMHeight() const { return (rotateView == VIEW_ROTATE_90 || rotateView == VIEW_ROTATE_270) ? viewSMMWidth_ : viewSMMHeight_; }
	#else
	GC viewSMMWidth() const { return viewMMWidth(); }
	GC viewSMMHeight() const { return viewMMHeight(); }
	#endif

	int xMMSizeToPixel(GC mm) const { return std::round(mm * mmToPixelXScaler); }
	int yMMSizeToPixel(GC mm) const { return std::round(mm * mmToPixelYScaler); }

	#ifdef CONFIG_BASE_ANDROID
	int xSMMSizeToPixel(GC mm) const { return std::round(mm * smmToPixelXScaler); }
	int ySMMSizeToPixel(GC mm) const { return std::round(mm * smmToPixelYScaler); }
	#else
	int xSMMSizeToPixel(GC mm) const { return xMMSizeToPixel(mm); }
	int ySMMSizeToPixel(GC mm) const { return yMMSizeToPixel(mm); }
	#endif

	bool isPortrait()
	{
		return viewPixelWidth() <  viewPixelHeight();
	}

	void adjustViewport(IG::Rect2<int> rect);

	// the raw window bounds, used when adjusting the viewport
	IG::Rect2<int> untransformedViewBounds() const;

	// the window bounds from the origin of the drawable area
	IG::Rect2<int> viewBounds() const
	{
		return IG::Rect2<int>(0, 0, (int)viewPixelWidth(), (int)viewPixelHeight());
	}

	IG::Rect2<int> relRectFromViewport(int newX, int newY, int xSize, int ySize, _2DOrigin posOrigin, _2DOrigin screenOrigin)
	{
		// adjust to the requested origin on the screen
		newX = LTIC2DO.adjustX(newX, (int)viewPixelWidth(), screenOrigin.invertYIfCartesian());
		newY = LTIC2DO.adjustY(newY, (int)viewPixelHeight(), screenOrigin.invertYIfCartesian());
		IG::Rect2<int> rect;
		rect.setPosRel({newX, newY}, xSize, ySize, posOrigin);
		return rect;
	}

	IG::Rect2<int> relRectFromViewport(int newX, int newY, int size, _2DOrigin posOrigin, _2DOrigin screenOrigin)
	{
		return relRectFromViewport(newX, newY, size, size, posOrigin, screenOrigin);
	}

	IG::Rect2<int> relRectFromViewport(int newX, int newY, IG::Point2D<int> size, _2DOrigin posOrigin, _2DOrigin screenOrigin)
	{
		return relRectFromViewport(newX, newY, size.x, size.y, posOrigin, screenOrigin);
	}

	IG::Point2D<int> sizesWithRatioBestFitFromViewport(float destAspectRatio)
	{
		return IG::sizesWithRatioBestFit(destAspectRatio, (int)viewPixelWidth(), (int)viewPixelHeight());
	}

	IG::Rect2<int> rectWithRatioBestFitFromViewport(int newX, int newY, float aR, _2DOrigin posOrigin, _2DOrigin screenOrigin)
	{
		return relRectFromViewport(newX, newY, sizesWithRatioBestFitFromViewport(aR), posOrigin, screenOrigin);
	}

	void updateSize(int width, int height);
	void calcPhysicalSize();

	uint setOrientation(uint o);
	uint setValidOrientations(uint oMask, bool manageAutoOrientation = 0);
	void setupScreenSize(); // called after DPI update in base module

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

	void displayNeedsUpdate();
	void unpostDraw();
	void postResize(bool redraw = true);
	void draw(Gfx::FrameTimeBase frameTime);
	void setAsDrawTarget();
	void swapBuffers();
};

}
