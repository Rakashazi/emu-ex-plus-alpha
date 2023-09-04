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

#include "InputDeviceData.hh"
#include <emuframework/AppKeyCode.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

constexpr SystemLogger log{"InputDevData"};

InputDeviceData::InputDeviceData(const InputManager &mgr, Input::Device &dev):
	devConf{dev},
	displayName{makeDisplayName(dev.name(), dev.enumId())}
{
	dev.setJoystickAxesAsDpad(Input::AxisSetId::stick1, true);
	dev.setJoystickAxesAsDpad(Input::AxisSetId::hat, true);
	for(auto &savedPtr : mgr.savedInputDevs)
	{
		if(savedPtr->matchesDevice(dev))
		{
			log.info("has saved config");
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
	log.info("allocating key mapping for:{} with player:{}", d.name(), devConf.player() + 1);
	actionTable = {totalKeys};
	keyCombos.clear();
	for(auto [key, mapKeys] : devConf.keyConf(mgr).keyMap)
	{
		if(!key.isAppKey())
			key = mgr.transpose(key, devConf.player());
		if(mapKeys.size() > 1)
		{
			bool hasModifierKeys{};
			keyCombos.emplace_back(key, mapKeys);
			for(auto mapKey : mapKeys)
			{
				if(d.isModifierKey(mapKey))
					hasModifierKeys = true;
				else
					actionTable[mapKey].tryInsert(actionTable[mapKey].begin(), KeyInfo::comboKey(keyCombos.size() - 1));
			}
			if(hasModifierKeys) // add extra mapping to account for left/right modifier keys
			{
				for(auto &mapKey : mapKeys)
					mapKey = d.swapModifierKey(mapKey);
				keyCombos.emplace_back(key, mapKeys);
				for(auto mapKey : mapKeys)
				{
					if(!d.isModifierKey(mapKey))
						actionTable[mapKey].tryInsert(actionTable[mapKey].begin(), KeyInfo::comboKey(keyCombos.size() - 1));
				}
			}
		}
		else
		{
			assert(mapKeys[0] < totalKeys);
			actionTable[mapKeys[0]].tryPushBack(key);
		}
	}
	/*for(auto [group, i] : std::views::zip(actionTable, iotaCount(actionTable.size())))
	{
		for(auto key : group)
		{
			if(!key)
				break;
			log.info("mapped key {} to {} ({})", d.keyName(i), mgr.toString(key), key.codes[0]);
		}
	}*/
}

std::string InputDeviceData::makeDisplayName(std::string_view name, int id)
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

void InputDeviceData::updateInputKey(const Input::KeyEvent &keyEv)
{
	if(keyEv.repeated())
		return;
	if(keyEv.pushed())
	{
		addInputKey(keyEv.mapKey());
	}
	else
	{
		removeInputKey(keyEv.mapKey());
	}
}

void InputDeviceData::addInputKey(Input::Key key)
{
	pushedInputKeys.push_back(key);
}

void InputDeviceData::removeInputKey(Input::Key key)
{
	std::ranges::replace(pushedInputKeys, key, Input::Key{});
}

bool InputDeviceData::keysArePushed(MappedKeys mapKeys)
{
	for(auto k : mapKeys)
	{
		if(!contains(pushedInputKeys, k))
			return false;
	}
	return true;
}

}
