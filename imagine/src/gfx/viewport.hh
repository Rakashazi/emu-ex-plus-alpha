#pragma once

#include <engine-globals.h>
#include <util/bits.h>

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
void setupScreenSize(); // called after DPI update in base module

}
