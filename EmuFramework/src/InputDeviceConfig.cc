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

#include "InputDeviceConfig.hh"
#include "InputDeviceData.hh"
#include <emuframework/EmuApp.hh>
#include <imagine/util/format.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

constexpr SystemLogger log{"InputDevConf"};

static StaticString<16> uniqueCustomConfigName(auto &customKeyConfigs)
{
	for(auto i : iotaCount(100)) // Try up to "Custom 99"
	{
		auto name = format<StaticString<16>>("Custom {}", i+1);
		std::string_view nameStr{name};
		// Check if this name is free
		log.info("checking:{}", name);
		if(!find(customKeyConfigs, [&](auto &ptr){return ptr->name == nameStr;}))
		{
			log.info("unique custom key config name:{}", name);
			return name;
		}
	}
	return {};
}

void InputDeviceConfig::deleteConf(InputManager& mgr)
{
	if(!savedConf)
		return;
	log.info("removing device config for {}", savedConf->name);
	std::erase_if(mgr.savedDevConfigs, [&](auto &ptr){ return ptr.get() == savedConf; });
	savedConf = nullptr;
}

void InputDeviceConfig::deleteSessionConf(InputManager& mgr)
{
	if(!sessionSavedConf)
		return;
	log.info("removing device session config for {}", sessionSavedConf->name);
	std::erase_if(mgr.savedSessionDevConfigs, [&](auto &ptr){ return ptr.get() == sessionSavedConf; });
	sessionSavedConf = nullptr;
}

void InputDeviceConfig::setICadeMode(bool on)
{
	dev->setICadeMode(on);
}

bool InputDeviceConfig::iCadeMode()
{
	return dev->iCadeMode();
}

bool InputDeviceConfig::joystickAxesAsKeys(Input::AxisSetId id)
{
	return dev->joystickAxesAsKeys(id);
}

void InputDeviceConfig::setJoystickAxesAsKeys(Input::AxisSetId id, bool on)
{
	dev->setJoystickAxesAsKeys(id, on);
}

void InputDeviceConfig::setKeyConfName(InputManager &mgr, std::string_view name)
{
	save(mgr).keyConfName = name;
	buildKeyMap(mgr);
}

void InputDeviceConfig::setSessionKeyConfName(InputManager &mgr, std::string_view name)
{
	saveSession(mgr).keyConfName = name;
	buildKeyMap(mgr);
}

InputDeviceSessionConfig InputDeviceConfig::sessionConfig(const InputManager &mgr) const
{
	InputDeviceSessionConfig conf;
	if(sessionSavedConf)
	{
		conf.keyConfName = sessionSavedConf->keyConfName;
		conf.player = sessionSavedConf->player;
	}
	if(!conf.keyConfName.size())
	{
		if(savedConf && savedConf->keyConfName.size())
		{
			conf.keyConfName = savedConf->keyConfName;
		}
		else
		{
			conf.keyConfName = mgr.defaultConfig(*dev).name;
		}
	}
	if(conf.player == playerIndexUnset)
	{
		conf.player = savedPlayer();
	}
	return conf;
}

KeyConfigDesc InputDeviceConfig::keyConf(const InputManager &mgr) const
{
	assert(dev);
	if(savedConf && savedConf->keyConfName.size())
	{
		//log.info("has saved config:{}", savedConf->keyConfName);
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
		log.info("current config not mutable, creating one");
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

InputDeviceSavedConfig& InputDeviceConfig::save(InputManager& mgr)
{
	if(!savedConf)
	{
		savedConf = mgr.savedDevConfigs.emplace_back(std::make_unique<InputDeviceSavedConfig>()).get();
		log.info("allocated new device config, {} total", mgr.savedDevConfigs.size());
	}
	savedConf->enabled = isEnabled;
	savedConf->enumId = dev->enumId();
	savedConf->joystickAxisAsDpadFlags.stick1 = dev->joystickAxesAsKeys(Input::AxisSetId::stick1);
	savedConf->joystickAxisAsDpadFlags.stick2 = dev->joystickAxesAsKeys(Input::AxisSetId::stick2);
	savedConf->joystickAxisAsDpadFlags.hat = dev->joystickAxesAsKeys(Input::AxisSetId::hat);
	savedConf->joystickAxisAsDpadFlags.triggers = dev->joystickAxesAsKeys(Input::AxisSetId::triggers);
	savedConf->joystickAxisAsDpadFlags.pedals = dev->joystickAxesAsKeys(Input::AxisSetId::pedals);
	savedConf->iCadeMode = dev->iCadeMode();
	savedConf->handleUnboundEvents = shouldHandleUnboundKeys;
	savedConf->name = dev->name();
	return *savedConf;
}

InputDeviceSavedSessionConfig& InputDeviceConfig::saveSession(InputManager& mgr)
{
	if(!sessionSavedConf)
	{
		sessionSavedConf = mgr.savedSessionDevConfigs.emplace_back(std::make_unique<InputDeviceSavedSessionConfig>()).get();
		log.info("allocated new device session config, {} total", mgr.savedSessionDevConfigs.size());
	}
	sessionSavedConf->enumId = dev->enumId();
	sessionSavedConf->name = dev->name();
	return *sessionSavedConf;
}

void InputDeviceConfig::setSavedConf(const InputManager &mgr, InputDeviceSavedConfig *savedConf, bool updateKeymap)
{
	this->savedConf = savedConf;
	if(savedConf)
	{
		isEnabled = savedConf->enabled;
		dev->setJoystickAxesAsKeys(Input::AxisSetId::stick1, savedConf->joystickAxisAsDpadFlags.stick1);
		dev->setJoystickAxesAsKeys(Input::AxisSetId::stick2, savedConf->joystickAxisAsDpadFlags.stick2);
		dev->setJoystickAxesAsKeys(Input::AxisSetId::hat, savedConf->joystickAxisAsDpadFlags.hat);
		dev->setJoystickAxesAsKeys(Input::AxisSetId::triggers, savedConf->joystickAxisAsDpadFlags.triggers);
		dev->setJoystickAxesAsKeys(Input::AxisSetId::pedals, savedConf->joystickAxisAsDpadFlags.pedals);
		dev->setICadeMode(savedConf->iCadeMode);
		shouldHandleUnboundKeys = savedConf->handleUnboundEvents;
	}
	else
	{
		isEnabled = true;
		dev->setJoystickAxesAsKeys(Input::AxisSetId::stick1, true);
		dev->setJoystickAxesAsKeys(Input::AxisSetId::hat, true);
		dev->setJoystickAxesAsKeys(Input::AxisSetId::triggers, true);
		dev->setJoystickAxesAsKeys(Input::AxisSetId::pedals, true);
		dev->setICadeMode(false);
		shouldHandleUnboundKeys = false;
	}
	if(updateKeymap)
		buildKeyMap(mgr);
}

void InputDeviceConfig::setSavedConf(const InputManager &mgr, InputDeviceSavedSessionConfig *savedConf, bool updateKeymap)
{
	this->sessionSavedConf = savedConf;
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

void InputDeviceConfig::setSavedPlayer(InputManager& mgr, int p)
{
	save(mgr).player = p;
	buildKeyMap(mgr);
}

void InputDeviceConfig::setSavedSessionPlayer(InputManager& mgr, int p)
{
	saveSession(mgr).player = p;
	buildKeyMap(mgr);
}

void InputDeviceConfig::buildKeyMap(const InputManager &mgr)
{
	inputDevData(*dev).buildKeyMap(mgr, *dev);
}

}
