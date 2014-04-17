#pragma once

#include <imagine/input/Device.hh>
#include <imagine/base/Window.hh>

namespace Input
{

void setAllowKeyRepeats(bool on);
bool allowKeyRepeats();
bool processICadeKey(char c, uint action, const Device &dev, Base::Window &win);

}
