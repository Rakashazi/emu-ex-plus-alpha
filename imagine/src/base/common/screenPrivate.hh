#pragma once

#include <imagine/base/Screen.hh>
#include <imagine/util/container/ArrayList.hh>

namespace Base
{

#ifdef CONFIG_BASE_MULTI_SCREEN
extern StaticArrayList<Screen*, 4> screen_;
#endif

}
