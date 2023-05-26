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

static StaticString<16> uniqueCustomConfigName(KeyConfigContainer &customKeyConfigs)
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
			if(ePtr->name == name)
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

void InputDeviceConfig::deleteConf(InputDeviceSavedConfigContainer &savedInputDevs)
{
	if(savedConf)
	{
		logMsg("removing device config for %s", savedConf->name.data());
		std::erase_if(savedInputDevs, [&](auto &ptr){ return ptr.get() == savedConf; });
		savedConf = nullptr;
	}
}

#ifdef CONFIG_INPUT_ICADE
bool InputDeviceConfig::setICadeMode(bool on, InputDeviceSavedConfigContainer &savedInputDevs)
{
	// delete device's config since its properties will change with iCade mode switch
	deleteConf(savedInputDevs);
	dev->setICadeMode(on);
	buildKeyMap();
	save(savedInputDevs);
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
#endif

unsigned InputDeviceConfig::joystickAxisAsDpadBits()
{
	return dev->joystickAxisAsDpadBits();
}

void InputDeviceConfig::setJoystickAxisAsDpadBits(unsigned axisMask)
{
	dev->setJoystickAxisAsDpadBits(axisMask);
}

const KeyConfig &InputDeviceConfig::keyConf() const
{
	if(savedConf && savedConf->keyConf)
	{
		//logMsg("has saved config %p", savedConf->keyConf_);
		return *savedConf->keyConf;
	}
	assert(dev);
	return KeyConfig::defaultConfigForDevice(*dev);
}

void InputDeviceConfig::setKeyConf(const KeyConfig &kConf, InputDeviceSavedConfigContainer &savedInputDevs)
{
	save(savedInputDevs);
	savedConf->keyConf = &kConf;
	buildKeyMap();
}

void InputDeviceConfig::setDefaultKeyConf()
{
	if(savedConf)
	{
		savedConf->keyConf = nullptr;
	}
}

KeyConfig *InputDeviceConfig::mutableKeyConf(KeyConfigContainer &customKeyConfigs) const
{
	auto currConf = &keyConf();
	//logMsg("curr key config %p", currConf);
	for(auto &e : customKeyConfigs)
	{
		//logMsg("checking key config %p", &e);
		if(e.get() == currConf)
		{
			return e.get();
		}
	}
	return nullptr;
}

KeyConfig *InputDeviceConfig::makeMutableKeyConf(EmuApp &app)
{
	auto &customKeyConfigs = app.customKeyConfigList();
	auto conf = mutableKeyConf(customKeyConfigs);
	if(!conf)
	{
		logMsg("current config not mutable, creating one");
		auto name = uniqueCustomConfigName(customKeyConfigs);
		conf = setKeyConfCopiedFromExisting(name, customKeyConfigs, app.savedInputDeviceList());
		app.postMessage(3, false, std::format("Automatically created profile: {}", conf->name));
	}
	return conf;
}

KeyConfig *InputDeviceConfig::setKeyConfCopiedFromExisting(std::string_view name,
	KeyConfigContainer &customKeyConfigs, InputDeviceSavedConfigContainer &savedInputDevs)
{
	auto &newConf = customKeyConfigs.emplace_back(std::make_unique<KeyConfig>(keyConf()));
	newConf->name = name;
	setKeyConf(*newConf, savedInputDevs);
	return newConf.get();
}

void InputDeviceConfig::save(InputDeviceSavedConfigContainer &savedInputDevs)
{
	if(!savedConf)
	{
		savedConf = savedInputDevs.emplace_back(std::make_unique<InputDeviceSavedConfig>()).get();
		logMsg("allocated new device config, %d total", (int)savedInputDevs.size());
	}
	savedConf->player = player_;
	savedConf->enabled = enabled;
	savedConf->enumId = dev->enumId();
	savedConf->joystickAxisAsDpadBits = dev->joystickAxisAsDpadBits();
	#ifdef CONFIG_INPUT_ICADE
	savedConf->iCadeMode = dev->iCadeMode();
	#endif
	savedConf->handleUnboundEvents = shouldConsumeUnboundKeys();
	savedConf->name = dev->name();
}

void InputDeviceConfig::setSavedConf(InputDeviceSavedConfig *savedConf, bool updateKeymap)
{
	this->savedConf = savedConf;
	if(savedConf)
	{
		player_ = savedConf->player;
		enabled = savedConf->enabled;
		dev->setJoystickAxisAsDpadBits(savedConf->joystickAxisAsDpadBits);
		#ifdef CONFIG_INPUT_ICADE
		dev->setICadeMode(savedConf->iCadeMode);
		#endif
		setConsumeUnboundKeys(savedConf->handleUnboundEvents);
	}
	else
	{
		player_ = dev->enumId() < EmuSystem::maxPlayers ? dev->enumId() : 0;
		enabled = true;
		dev->setJoystickAxisAsDpadBits(Input::Device::AXIS_BITS_STICK_1 | Input::Device::AXIS_BITS_HAT);
		#ifdef CONFIG_INPUT_ICADE
		dev->setICadeMode(false);
		#endif
		setConsumeUnboundKeys(false);
	}
	if(updateKeymap)
		buildKeyMap();
}

bool InputDeviceConfig::setKey(EmuApp &app, Input::Key mapKey, const KeyCategory &cat, int keyIdx)
{
	auto conf = makeMutableKeyConf(app);
	if(!conf)
		return false;
	auto &keyEntry = conf->key(cat)[keyIdx];
	logMsg("changing key mapping from %s (0x%X) to %s (0x%X)",
			dev->keyName(keyEntry), keyEntry, dev->keyName(mapKey), mapKey);
	keyEntry = mapKey;
	return true;
}

void InputDeviceConfig::setPlayer(int p)
{
	player_ = p;
	buildKeyMap();
}

void InputDeviceConfig::buildKeyMap()
{
	inputDevData(*dev).buildKeyMap(*dev);
}

}
