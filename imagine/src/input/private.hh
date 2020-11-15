#pragma once

#include <imagine/input/Input.hh>
#include <imagine/input/Device.hh>

namespace Base
{
class Window;
}

namespace Input
{

struct PackedInputAccess
{
	uint32_t byteOffset;
	uint32_t mask;
	Key keyEvent;
	Key sysKey;

	int updateState(const uint8_t *prev, const uint8_t *curr) const
	{
		bool oldState = prev[byteOffset] & mask,
			newState = curr[byteOffset] & mask;
		if(oldState != newState)
		{
			return newState;
		}
		return -1; // no state change
	}
};

extern std::vector<Device*> devList;
extern DeviceChangeDelegate onDeviceChange;
extern DevicesEnumeratedDelegate onDevicesEnumerated;
extern bool swappedGamepadConfirm_;

bool processICadeKey(Key k, uint32_t action, Time time, const Device &dev, Base::Window &win);

}
