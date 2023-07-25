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

#define LOGTAG "InputDevConf"
#include "privateInput.hh"
#include <emuframework/EmuApp.hh>
#include <imagine/util/format.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

static StaticString<16> uniqueCustomConfigName(auto &customKeyConfigs)
{
	for(auto i : iotaCount(100)) // Try up to "Custom 99"
	{
		auto name = format<StaticString<16>>("Custom {}", i+1);
		// Check if this name is free
		logMsg("checking:%s", name.data());
		bool exists{};
		for(auto &ePtr : customKeyConfigs)
		{
			logMsg("against:%s", ePtr->name.data());
			if(ePtr->name == std::string_view{name})
			{
				exists = true;
				break;
			}
		}
		if(!exists)
		{
			logMsg("unique custom key config name: %s", name.data());
			return name;
		}
	}
	return {};
}

void InputDeviceConfig::deleteConf(InputManager &mgr)
{
	if(!savedConf)
		return;
	logMsg("removing device config for %s", savedConf->name.data());
	std::erase_if(mgr.savedInputDevs, [&](auto &ptr){ return ptr.get() == savedConf; });
	savedConf = nullptr;
}

bool InputDeviceConfig::setICadeMode(InputManager &mgr, bool on)
{
	// delete device's config since its properties will change with iCade mode switch
	deleteConf(mgr);
	dev->setICadeMode(on);
	buildKeyMap(mgr);
	save(mgr);
	if(!savedConf)
	{
		logErr("can't save iCade mode");
		return 0;
	}
	savedConf->iCadeMode = on;
	return 1;
}

bool InputDeviceConfig::iCadeMode()
{
	return dev->iCadeMode();
}

unsigned InputDeviceConfig::joystickAxisAsDpadBits()
{
	return dev->joystickAxisAsDpadBits();
}

void InputDeviceConfig::setJoystickAxisAsDpadBits(unsigned axisMask)
{
	dev->setJoystickAxisAsDpadBits(axisMask);
}

void InputDeviceConfig::setKeyConfName(InputManager &mgr, std::string_view name)
{
	save(mgr);
	if(name.size() > 255) // truncate if name is too long for config file
	{
		name.remove_suffix(name.size() - 255);
	}
	savedConf->keyConfName = name;
	buildKeyMap(mgr);
}

KeyConfigDesc InputDeviceConfig::keyConf(const InputManager &mgr) const
{
	assert(dev);
	if(savedConf && savedConf->keyConfName.size())
	{
		//logMsg("has saved config:%s", savedConf->keyConfName.c_str());
		auto conf = mgr.keyConfig(savedConf->keyConfName, *dev);
		if(conf)
			return conf;
	}
	return mgr.defaultConfig(*dev);
}

void InputDeviceConfig::setDefaultKeyConf()
{
	if(!savedConf)
		return;
	savedConf->keyConfName.clear();
}

KeyConfig *InputDeviceConfig::mutableKeyConf(InputManager &mgr) const
{
	if(!savedConf)
		return {};
	return mgr.customKeyConfig(savedConf->keyConfName, *dev);
}

KeyConfig *InputDeviceConfig::makeMutableKeyConf(EmuApp &app)
{
	auto &mgr = app.inputManager;
	auto conf = mutableKeyConf(mgr);
	if(!conf)
	{
		logMsg("current config not mutable, creating one");
		auto name = uniqueCustomConfigName(mgr.customKeyConfigs);
		conf = setKeyConfCopiedFromExisting(mgr, name);
		app.postMessage(3, false, std::format("Automatically created profile: {}", conf->name));
	}
	return conf;
}

KeyConfig *InputDeviceConfig::setKeyConfCopiedFromExisting(InputManager &mgr, std::string_view name)
{
	auto &newConf = mgr.customKeyConfigs.emplace_back(std::make_unique<KeyConfig>(keyConf(mgr)));
	newConf->name = name;
	setKeyConfName(mgr, name);
	return newConf.get();
}

void InputDeviceConfig::save(InputManager &mgr)
{
	if(!savedConf)
	{
		savedConf = mgr.savedInputDevs.emplace_back(std::make_unique<InputDeviceSavedConfig>()).get();
		logMsg("allocated new device config, %d total", (int)mgr.savedInputDevs.size());
	}
	savedConf->player = player_;
	savedConf->enabled = isEnabled;
	savedConf->enumId = dev->enumId();
	savedConf->joystickAxisAsDpadBits = dev->joystickAxisAsDpadBits();
	savedConf->iCadeMode = dev->iCadeMode();
	savedConf->handleUnboundEvents = shouldHandleUnboundKeys;
	savedConf->name = dev->name();
}

void InputDeviceConfig::setSavedConf(const InputManager &mgr, InputDeviceSavedConfig *savedConf, bool updateKeymap)
{
	this->savedConf = savedConf;
	if(savedConf)
	{
		player_ = savedConf->player;
		isEnabled = savedConf->enabled;
		dev->setJoystickAxisAsDpadBits(savedConf->joystickAxisAsDpadBits);
		dev->setICadeMode(savedConf->iCadeMode);
		shouldHandleUnboundKeys = savedConf->handleUnboundEvents;
	}
	else
	{
		player_ = dev->enumId() < EmuSystem::maxPlayers ? dev->enumId() : 0;
		isEnabled = true;
		dev->setJoystickAxisAsDpadBits(Input::Device::AXIS_BITS_STICK_1 | Input::Device::AXIS_BITS_HAT);
		dev->setICadeMode(false);
		shouldHandleUnboundKeys = false;
	}
	if(updateKeymap)
		buildKeyMap(mgr);
}

bool InputDeviceConfig::setKey(EmuApp &app, KeyInfo code, MappedKeys mapKey)
{
	auto conf = makeMutableKeyConf(app);
	if(!conf)
		return false;
	conf->set(code, mapKey);
	return true;
}

void InputDeviceConfig::setPlayer(const InputManager &mgr, int p)
{
	player_ = p;
	buildKeyMap(mgr);
}

void InputDeviceConfig::buildKeyMap(const InputManager &mgr)
{
	inputDevData(*dev).buildKeyMap(mgr, *dev);
}

}
