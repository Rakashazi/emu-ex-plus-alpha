#pragma once

#include <engine-globals.h>
#include <util/bits.h>
#include <util/rectangle2.h>

namespace Gfx
{

// orientation
static const uint VIEW_ROTATE_0 = BIT(0), VIEW_ROTATE_90 = BIT(1), VIEW_ROTATE_180 = BIT(2), VIEW_ROTATE_270 = BIT(3);
static const uint VIEW_ROTATE_AUTO = BIT(5);

#ifdef CONFIG_GFX_SOFT_ORIENTATION
extern uint rotateView;
extern uint preferedOrientation;
#else
static const uint rotateView = VIEW_ROTATE_0;
static const uint preferedOrientation = VIEW_ROTATE_0;
#endif

uint setOrientation(uint o);
uint setValidOrientations(uint oMask, bool manageAutoOrientation = 0);
static const char *orientationName(uint o)
{
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

#ifdef CONFIG_INPUT
	void configureInputForOrientation();
#endif

// pixel buffer size
extern uint viewPixelWidth_, viewPixelHeight_;
static uint viewPixelWidth() { return (rotateView == VIEW_ROTATE_90 || rotateView == VIEW_ROTATE_270) ? viewPixelHeight_ : viewPixelWidth_; }
static uint viewPixelHeight() { return (rotateView == VIEW_ROTATE_90 || rotateView == VIEW_ROTATE_270) ? viewPixelWidth_ : viewPixelHeight_; }

// pixel density
extern uint viewMMWidth_, viewMMHeight_; // in MM
static uint viewMMWidth() { return (rotateView == VIEW_ROTATE_90 || rotateView == VIEW_ROTATE_270) ? viewMMHeight_ : viewMMWidth_; }
static uint viewMMHeight() { return (rotateView == VIEW_ROTATE_90 || rotateView == VIEW_ROTATE_270) ? viewMMWidth_ : viewMMHeight_; }
#ifdef CONFIG_BASE_ANDROID
extern uint viewSMMWidth_, viewSMMHeight_; // in MM from scaled screen units
static uint viewSMMWidth() { return (rotateView == VIEW_ROTATE_90 || rotateView == VIEW_ROTATE_270) ? viewSMMHeight_ : viewSMMWidth_; }
static uint viewSMMHeight() { return (rotateView == VIEW_ROTATE_90 || rotateView == VIEW_ROTATE_270) ? viewSMMWidth_ : viewSMMHeight_; }
#else
static uint viewSMMWidth() { return viewMMWidth(); }
static uint viewSMMHeight() { return viewMMHeight(); }
#endif
void setupScreenSize(); // called after DPI update in base module

// shortcuts for creating rectangles with origins in a viewport
static Rect2<int> relRectFromViewport(int newX, int newY, int xSize, int ySize, _2DOrigin posOrigin, _2DOrigin screenOrigin)
{
	// adjust to the requested origin on the screen
	newX = LTIC2DO.adjustX(newX, (int)Gfx::viewPixelWidth(), screenOrigin.invertYIfCartesian());
	newY = LTIC2DO.adjustY(newY, (int)Gfx::viewPixelHeight(), screenOrigin.invertYIfCartesian());
	Rect2<int> rect;
	rect.setPosRel(newX, newY, xSize, ySize, posOrigin);
	return rect;
}

static Rect2<int> relRectFromViewport(int newX, int newY, int size, _2DOrigin posOrigin, _2DOrigin screenOrigin)
{
	return relRectFromViewport(newX, newY, size, size, posOrigin, screenOrigin);
}

static Rect2<int> relRectFromViewport(int newX, int newY, IG::Point2D<int> size, _2DOrigin posOrigin, _2DOrigin screenOrigin)
{
	return relRectFromViewport(newX, newY, size.x, size.y, posOrigin, screenOrigin);
}

static IG::Point2D<int> sizesWithRatioBestFitFromViewport(float destAspectRatio)
{
	return IG::sizesWithRatioBestFit(destAspectRatio, (int)viewPixelWidth(), (int)viewPixelHeight());
}

static Rect2<int> rectWithRatioBestFitFromViewport(int newX, int newY, Rational aR, _2DOrigin posOrigin, _2DOrigin screenOrigin)
{
	return relRectFromViewport(newX, newY, sizesWithRatioBestFitFromViewport((float)aR), posOrigin, screenOrigin);
}

}
