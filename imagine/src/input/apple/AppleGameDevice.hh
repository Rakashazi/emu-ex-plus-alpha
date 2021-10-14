#pragma once

#include <imagine/input/config.hh>
#include <utility>

namespace Base
{
class ApplicationContext;
}

namespace Input
{
void initAppleGameControllers(Base::ApplicationContext);
std::pair<Input::Key, Input::Key> appleJoystickKeys(Input::AxisId);
}
