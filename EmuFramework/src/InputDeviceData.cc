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
#include <emuframework/AppKeyCode.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

InputDeviceData::InputDeviceData(const InputManager &mgr, Input::Device &dev):
	devConf{dev},
	displayName{makeDisplayName(dev.name(), dev.enumId())}
{
	dev.setJoystickAxisAsDpadBits(Input::Device::AXIS_BITS_STICK_1 | Input::Device::AXIS_BITS_HAT);
	for(auto &savedPtr : mgr.savedInputDevs)
	{
		if(savedPtr->matchesDevice(dev))
		{
			logMsg("has saved config");
			devConf.setSavedConf(mgr, savedPtr.get(), false);
		}
	}
	buildKeyMap(mgr, dev);
}

void InputDeviceData::buildKeyMap(const InputManager &mgr, const Input::Device &d)
{
	auto totalKeys = Input::KeyEvent::mapNumKeys(d.map());
	if(!totalKeys || !devConf.isEnabled) [[unlikely]]
		return;
	logMsg("allocating key mapping for:%s with player:%d", d.name().data(), devConf.player()+1);
	actionTable = {totalKeys};
	for(auto [key, value] : devConf.keyConf(mgr).keyMap)
	{
		if(!key.isAppKey())
			key = mgr.transpose(key, devConf.player());
		for(auto mapKey : value)
		{
			logMsg("mapping key %s to %s (%u)", d.keyName(mapKey), mgr.toString(key).data(), key.codes[0]);
			assert(mapKey < totalKeys);
			actionTable[mapKey].tryPushBack(key);
		}
	}
	/*for(auto [group, i] : std::views::zip(actionTable, iotaCount(actionTable.size())))
	{
		for(auto code : group)
		{
			if(!code)
				break;
			logMsg("key %s (%u) mapped as %s", mgr.keyCodeToString(code).data(), code, d.keyName(i));
		}
	}*/
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
