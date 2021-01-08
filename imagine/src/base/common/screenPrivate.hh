#pragma once

#include <imagine/base/Screen.hh>

namespace Base
{

#ifdef CONFIG_BASE_MULTI_SCREEN
extern std::vector<std::unique_ptr<Screen>> screen_;
#endif

}
