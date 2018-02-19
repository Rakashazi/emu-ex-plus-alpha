#pragma once

#include <imagine/base/Window.hh>
#include <vector>

namespace Base
{

#ifdef CONFIG_BASE_MULTI_WINDOW
extern std::vector<Window*> window_;
#else
extern Window *mainWin;
#endif

}
