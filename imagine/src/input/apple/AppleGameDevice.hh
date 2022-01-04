#pragma once

#include <imagine/input/config.hh>
#include <utility>

namespace IG
{
class ApplicationContext;
}

namespace IG::Input
{
void initAppleGameControllers(ApplicationContext);
std::pair<Input::Key, Input::Key> appleJoystickKeys(Input::AxisId);
}
