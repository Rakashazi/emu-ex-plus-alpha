#pragma once

#include <imagine/base/Screen.hh>
#include <vector>

namespace Base
{

#ifdef CONFIG_BASE_MULTI_SCREEN
extern std::vector<Screen*> screen_;
#endif

}
