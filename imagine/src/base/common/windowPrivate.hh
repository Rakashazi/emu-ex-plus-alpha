#pragma once

#include <base/Window.hh>
#include <util/collection/ArrayList.hh>

namespace Base
{

#ifdef CONFIG_BASE_MULTI_WINDOW
extern StaticArrayList<Window*, 4> window;
#else
extern Window *mainWin;
#endif

bool windowsArePosted();
bool drawWindows(Gfx::FrameTimeBase frameTime);

}
