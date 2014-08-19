#pragma once

#include <imagine/base/Window.hh>
#include <imagine/util/container/ArrayList.hh>

namespace Base
{

extern OnGLDrawableChangedDelegate onGLDrawableChanged;

#ifdef CONFIG_BASE_MULTI_WINDOW
extern StaticArrayList<Window*, 4> window_;
#else
extern Window *mainWin;
#endif

}
