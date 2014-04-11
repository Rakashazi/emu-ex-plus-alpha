#pragma once

#include <imagine/base/Window.hh>
#include <imagine/util/container/ArrayList.hh>

namespace Base
{

#ifdef CONFIG_BASE_MULTI_WINDOW
extern StaticArrayList<Window*, 4> window;
#else
extern Window *mainWin;
#endif

bool frameUpdate(FrameTimeBase frameTime, bool forceDraw);
static bool frameUpdate(FrameTimeBase frameTime)
{
	return frameUpdate(frameTime, false);
}
void clearOnFrameDelegates();

}
