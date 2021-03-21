#pragma once

#include <imagine/base/Window.hh>
#include <memory>

namespace Base
{

void deinitWindows();
std::unique_ptr<Window> moveOutWindow(Window &win);

}
