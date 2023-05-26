/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "InputDevData"
#include "privateInput.hh"
#include <imagine/logger/logger.h>

namespace EmuEx
{

InputDeviceData::InputDeviceData(Input::Device &dev, InputDeviceSavedConfigContainer &savedInputDevs):
	devConf{dev},
	displayName{makeDisplayName(dev.name(), dev.enumId())}
{
	dev.setJoystickAxisAsDpadBits(Input::Device::AXIS_BITS_STICK_1 | Input::Device::AXIS_BITS_HAT);
	for(auto &savedPtr : savedInputDevs)
	{
		if(savedPtr->matchesDevice(dev))
		{
			logMsg("has saved config");
			devConf.setSavedConf(savedPtr.get(), false);
		}
	}
	buildKeyMap(dev);
}

void InputDeviceData::buildKeyMap(const Input::Device &d)
{
	auto totalKeys = Input::KeyEvent::mapNumKeys(d.map());
	if(!totalKeys || !devConf.isEnabled()) [[unlikely]]
		return;
	logMsg("allocating key mapping for:%s with player:%d", d.name().data(), devConf.player()+1);
	actionTable = {totalKeys};
	KeyConfig::KeyArray key = devConf.keyConf().key();
	if(devConf.player() != InputDeviceConfig::PLAYER_MULTI)
	{
		Controls::transposeKeysForPlayer(key, devConf.player());
	}
	for(auto k : iotaCount(MAX_KEY_CONFIG_KEYS))
	{
		//logMsg("mapping key %d to %u %s", k, key[k], d.keyName(key[k]));
		assert(key[k] < totalKeys);
		auto &group = actionTable[key[k]];
		auto slot = std::ranges::find_if(group, [](auto &a){ return a == 0; });
		if(slot != group.end())
			*slot = k+1; // add 1 to avoid 0 value (considered unmapped)
	}
}

std::string InputDeviceData::makeDisplayName(std::string_view name, unsigned id)
{
	if(id)
	{
		return std::format("{} #{}", name, id + 1);
	}
	else
	{
		return std::string{name};
	}
}

}
