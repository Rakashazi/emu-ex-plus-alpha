#pragma once

#include <imagine/input/Input.hh>
#include <imagine/base/Window.hh>

namespace Input
{

extern std::vector<Device*> devList;
extern DeviceChangeDelegate onDeviceChange;
extern DevicesEnumeratedDelegate onDevicesEnumerated;
extern bool swappedGamepadConfirm_;

void setAllowKeyRepeats(bool on);
bool allowKeyRepeats();
bool processICadeKey(Key k, uint32_t action, Time time, const Device &dev, Base::Window &win);

}
