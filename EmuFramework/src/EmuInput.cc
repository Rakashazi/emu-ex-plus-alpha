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

#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuApp.hh>
#include "privateInput.hh"
#include "EmuOptions.hh"

namespace EmuEx
{

void InputManager::updateInputDevices(ApplicationContext ctx)
{
	for(auto &devPtr : ctx.inputDevices())
	{
		logMsg("input device:%s, id:%d, map:%d", devPtr->name().data(), devPtr->enumId(), (int)devPtr->map());
		auto &appData = devPtr->makeAppData<InputDeviceData>(*devPtr, savedInputDevs);
	}
	vController.setPhysicalControlsPresent(ctx.keyInputIsPresent());
	onUpdateDevices.callCopySafe();
}

void InputManager::writeCustomKeyConfigs(FileIO &io)
{
	if(!customKeyConfigs.size())
		return;
	auto categories = EmuApp::inputControlCategories();
	bool writeCategory[customKeyConfigs.size()][categories.size()];
	uint8_t writeCategories[customKeyConfigs.size()];
	std::fill_n(writeCategories, customKeyConfigs.size(), 0);
	// compute total size
	static_assert(sizeof(KeyConfig::name) <= 255, "key config name array is too large");
	size_t bytes = 2; // config key size
	bytes += 1; // number of configs
	for(auto [i, ePtr] : enumerate(customKeyConfigs))
	{
		auto &e = *ePtr;
		bytes += 1; // input map type
		bytes += 1; // name string length
		bytes += e.name.size(); // name string
		bytes += 1; // number of categories present
		for(auto &cat : EmuApp::inputControlCategories())
		{
			bool write{};
			const auto key = e.key(cat);
			for(auto k : iotaCount(cat.keys()))
			{
				if(key[k]) // check if category has any keys defined
				{
					write = true;
					break;
				}
			}
			writeCategory[i][std::distance(EmuApp::inputControlCategories().data(), &cat)] = write;
			if(!write)
			{
				logMsg("category:%s of key conf:%s skipped", cat.name.data(), e.name.data());
				continue;
			}
			writeCategories[i]++;
			bytes += 1; // category index
			bytes += 2; // category key array size
			bytes += cat.keys() * sizeof(KeyConfig::Key); // keys array
		}
	}
	if(bytes > 0xFFFF)
	{
		bug_unreachable("excessive key config size, should not happen");
	}
	// write to config file
	logMsg("saving %d key configs, %zu bytes", (int)customKeyConfigs.size(), bytes);
	io.put(uint16_t(bytes));
	io.put(uint16_t(CFGKEY_INPUT_KEY_CONFIGS));
	io.put(uint8_t(customKeyConfigs.size()));
	for(auto [i, ePtr] : enumerate(customKeyConfigs))
	{
		auto &e = *ePtr;
		logMsg("writing config %s", e.name.data());
		io.put(uint8_t(e.map));
		uint8_t nameLen = e.name.size();
		io.put(nameLen);
		io.write(e.name.data(), nameLen);
		io.put(writeCategories[i]);
		for(auto &cat : EmuApp::inputControlCategories())
		{
			uint8_t catIdx = std::distance(EmuApp::inputControlCategories().data(), &cat);
			if(!writeCategory[i][catIdx])
				continue;
			io.put(uint8_t(catIdx));
			uint16_t catSize = cat.keys() * sizeof(KeyConfig::Key);
			io.put(catSize);
			io.write(static_cast<void*>(e.key(cat)), catSize);
		}
	}
}

void InputManager::writeSavedInputDevices(FileIO &io)
{
	if(!savedInputDevs.size())
		return;
	// compute total size
	unsigned bytes = 2; // config key size
	bytes += 1; // number of configs
	for(auto &ePtr : savedInputDevs)
	{
		auto &e = *ePtr;
		bytes += 1; // device id
		bytes += 1; // enabled
		bytes += 1; // player
		bytes += 1; // mapJoystickAxis1ToDpad
		#ifdef CONFIG_INPUT_ICADE
		bytes += 1; // iCade mode
		#endif
		bytes += 1; // name string length
		bytes += std::min((size_t)256, e.name.size()); // name string
		bytes += 1; // key config map
		if(e.keyConf)
		{
			bytes += 1; // name of key config string length
			bytes += e.keyConf->name.size(); // name of key config string
		}
	}
	if(bytes > 0xFFFF)
	{
		bug_unreachable("excessive input device config size, should not happen");
	}
	// write to config file
	logMsg("saving %d input device configs, %d bytes", (int)savedInputDevs.size(), bytes);
	io.put(uint16_t(bytes));
	io.put(uint16_t(CFGKEY_INPUT_DEVICE_CONFIGS));
	io.put(uint8_t(savedInputDevs.size()));
	for(auto &ePtr : savedInputDevs)
	{
		auto &e = *ePtr;
		logMsg("writing config %s, id %d", e.name.data(), e.enumId);
		uint8_t enumIdWithFlags = e.enumId;
		if(e.handleUnboundEvents)
			enumIdWithFlags |= e.HANDLE_UNBOUND_EVENTS_FLAG;
		io.put(uint8_t(enumIdWithFlags));
		io.put(uint8_t(e.enabled));
		io.put(uint8_t(e.player));
		io.put(uint8_t(e.joystickAxisAsDpadBits));
		#ifdef CONFIG_INPUT_ICADE
		io.put(uint8_t(e.iCadeMode));
		#endif
		uint8_t nameLen = std::min((size_t)256, e.name.size());
		io.put(nameLen);
		io.write(e.name.data(), nameLen);
		uint8_t keyConfMap = e.keyConf ? (uint8_t)e.keyConf->map : 0;
		io.put(keyConfMap);
		if(keyConfMap)
		{
			logMsg("has key conf %s, map %d", e.keyConf->name.data(), keyConfMap);
			uint8_t keyConfNameLen = e.keyConf->name.size();
			io.put(keyConfNameLen);
			io.write(e.keyConf->name.data(), keyConfNameLen);
		}
	}
}

bool InputManager::readCustomKeyConfigs(MapIO &io, size_t size, std::span<const KeyCategory> categorySpan)
{
	static constexpr int KEY_CONFIGS_HARD_LIMIT = 256;
	auto confs = io.get<uint8_t>(); // TODO: unused currently, use to pre-allocate memory for configs
	size--;
	if(!size)
		return false;

	while(size)
	{
		KeyConfig keyConf{};

		keyConf.map = (Input::Map)io.get<uint8_t>();
		size--;
		if(!size)
			return false;

		auto nameLen = io.get<uint8_t>();
		size--;
		if(size < nameLen)
			return false;

		if(io.readSized(keyConf.name, nameLen) != nameLen)
			return false;
		size -= nameLen;
		if(!size)
			return false;

		auto categories = io.get<uint8_t>();
		size--;
		if(categories > categorySpan.size())
		{
			return false;
		}

		for(auto i : iotaCount(categories))
		{
			if(!size)
				return false;

			auto categoryIdx = io.get<uint8_t>();
			size--;
			if(categoryIdx >= categorySpan.size())
			{
				return false;
			}
			if(size < 2)
			{
				return false;
			}
			auto &cat = categorySpan[categoryIdx];
			auto catSize = io.get<uint16_t>();
			size -= 2;
			if(size < catSize)
				return false;

			if(catSize > cat.keys() * sizeof(KeyConfig::Key))
				return false;
			auto key = keyConf.key(cat);
			if(io.read(static_cast<void*>(key), catSize) != catSize)
				return false;
			size -= catSize;

			// verify keys
			{
				const auto keyMax = Input::KeyEvent::mapNumKeys(keyConf.map);
				for(auto i : iotaCount(cat.keys()))
				{
					if(key[i] >= keyMax)
					{
						logWarn("key code 0x%X out of range for map type %d", key[i], (int)keyConf.map);
						key[i] = 0;
					}
				}
			}

			logMsg("read category %d", categoryIdx);
		}

		logMsg("read key config %s", keyConf.name.data());
		customKeyConfigs.emplace_back(std::make_unique<KeyConfig>(keyConf));

		if(customKeyConfigs.size() == KEY_CONFIGS_HARD_LIMIT)
		{
			logWarn("reached custom key config hard limit:%d", KEY_CONFIGS_HARD_LIMIT);
			break;
		}
	}
	return true;
}

bool InputManager::readSavedInputDevices(MapIO &io, size_t size)
{
	static constexpr int INPUT_DEVICE_CONFIGS_HARD_LIMIT = 256;
	auto confs = io.get<uint8_t>(); // TODO: unused currently, use to pre-allocate memory for configs
	size--;
	if(!size)
		return false;

	while(size)
	{
		InputDeviceSavedConfig devConf;

		auto enumIdWithFlags = io.get<uint8_t>();
		size--;
		if(!size)
			return false;
		devConf.handleUnboundEvents = enumIdWithFlags & devConf.HANDLE_UNBOUND_EVENTS_FLAG;
		devConf.enumId = enumIdWithFlags & devConf.ENUM_ID_MASK;

		devConf.enabled = io.get<uint8_t>();
		size--;
		if(!size)
			return false;

		devConf.player = io.get<uint8_t>();
		if(devConf.player != InputDeviceConfig::PLAYER_MULTI && devConf.player > EmuSystem::maxPlayers)
		{
			logWarn("player %d out of range", devConf.player);
			devConf.player = 0;
		}
		size--;
		if(!size)
			return false;

		devConf.joystickAxisAsDpadBits = io.get<uint8_t>();
		size--;
		if(!size)
			return false;

		#ifdef CONFIG_INPUT_ICADE
		devConf.iCadeMode = io.get<uint8_t>();
		size--;
		if(!size)
			return false;
		#endif

		auto nameLen = io.get<uint8_t>();
		size--;
		if(size < nameLen)
			return false;

		io.readSized(devConf.name, nameLen);
		size -= nameLen;
		if(!size)
			return false;

		auto keyConfMap = Input::validateMap(io.get<uint8_t>());
		size--;

		if(keyConfMap != Input::Map::UNKNOWN)
		{
			if(!size)
				return false;

			auto keyConfNameLen = io.get<uint8_t>();
			size--;
			if(size < keyConfNameLen)
				return false;

			char keyConfName[keyConfNameLen + 1];
			if(io.read(keyConfName, keyConfNameLen) != keyConfNameLen)
				return false;
			keyConfName[keyConfNameLen] = '\0';
			size -= keyConfNameLen;

			for(auto &ePtr : customKeyConfigs)
			{
				auto &e = *ePtr;
				if(e.map == keyConfMap && e.name == keyConfName)
				{
					logMsg("found referenced custom key config %s while reading input device config", keyConfName);
					devConf.keyConf = &e;
					break;
				}
			}

			if(!devConf.keyConf) // check built-in configs after user-defined ones
			{
				for(const auto &conf : KeyConfig::defaultConfigsForInputMap(keyConfMap))
				{
					if(conf.name == keyConfName)
					{
						logMsg("found referenced built-in key config %s while reading input device config", keyConfName);
						devConf.keyConf = &conf;
						break;
					}
				}
			}
		}

		if(!IG::containsIf(savedInputDevs, [&](const auto &confPtr){ return *confPtr == devConf;}))
		{
			logMsg("read input device config:%s, id:%d", devConf.name.data(), devConf.enumId);
			savedInputDevs.emplace_back(std::make_unique<InputDeviceSavedConfig>(devConf));
		}
		else
		{
			logMsg("ignoring duplicate input device config:%s, id:%d", devConf.name.data(), devConf.enumId);
		}

		if(savedInputDevs.size() == INPUT_DEVICE_CONFIGS_HARD_LIMIT)
		{
			logWarn("reached input device config hard limit:%d", INPUT_DEVICE_CONFIGS_HARD_LIMIT);
			return false;
		}
	}
	return true;
}


// KeyConfig

const KeyConfig &KeyConfig::defaultConfigForDevice(const Input::Device &dev)
{
	switch(dev.map())
	{
		default: return defaultConfigsForDevice(dev)[0];
		case Input::Map::SYSTEM:
		{
			if constexpr(Config::envIsAndroid || Config::envIsLinux)
			{
				for(const auto &c : defaultConfigsForDevice(dev))
				{
					// Look for the first config to match the device subtype
					if(dev.subtype() == c.devSubtype)
					{
						return c;
					}
				}
			}
			return defaultConfigsForDevice(dev)[0];
		}
	}
}

std::span<const KeyConfig> KeyConfig::defaultConfigsForInputMap(Input::Map map)
{
	switch(map)
	{
		default: return {};
		case Input::Map::SYSTEM:
			return {Controls::defaultKeyProfile, Controls::defaultKeyProfiles};
		#ifdef CONFIG_INPUT_BLUETOOTH
		case Input::Map::WIIMOTE:
			return {Controls::defaultWiimoteProfile, Controls::defaultWiimoteProfiles};
		case Input::Map::WII_CC:
			return {Controls::defaultWiiCCProfile, Controls::defaultWiiCCProfiles};
		case Input::Map::ICONTROLPAD:
			return {Controls::defaultIControlPadProfile, Controls::defaultIControlPadProfiles};
		case Input::Map::ZEEMOTE:
			return {Controls::defaultZeemoteProfile, Controls::defaultZeemoteProfiles};
		#endif
		#ifdef CONFIG_BLUETOOTH_SERVER
		case Input::Map::PS3PAD:
			return {Controls::defaultPS3Profile, Controls::defaultPS3Profiles};
		#endif
		#ifdef CONFIG_INPUT_ICADE
		case Input::Map::ICADE:
			return {Controls::defaultICadeProfile, Controls::defaultICadeProfiles};
		#endif
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case Input::Map::APPLE_GAME_CONTROLLER:
			return {Controls::defaultAppleGCProfile, Controls::defaultAppleGCProfiles};
		#endif
	}
}

std::span<const KeyConfig> KeyConfig::defaultConfigsForDevice(const Input::Device &dev)
{
	auto conf = defaultConfigsForInputMap(dev.map());
	if(!conf.data())
	{
		bug_unreachable("device map:%d missing default configs", (int)dev.map());
	}
	return conf;
}

namespace Controls
{

void generic2PlayerTranspose(KeyConfig::KeyArray &key, int player, int startCategory)
{
	if(player == 0)
	{
		// clear P2 joystick keys
		auto cat2 = categories()[startCategory+1];
		std::fill_n(&key[cat2.configOffset], cat2.keys(), 0);
	}
	else
	{
		// transpose joystick keys
		auto cat = categories()[startCategory];
		auto cat2 = categories()[startCategory+1];
		std::copy_n(&key[cat.configOffset], cat.keys(), &key[cat2.configOffset]);
		std::fill_n(&key[cat.configOffset], cat.keys(), 0);
	}
}

void genericMultiplayerTranspose(KeyConfig::KeyArray &key, int player, int startCategory)
{
	for(int i : iotaCount(EmuSystem::maxPlayers))
	{
		if(player && i == player)
		{
			//logMsg("moving to player %d map", i);
			auto cat = categories()[startCategory];
			auto cat2 = categories()[startCategory+i];
			std::copy_n(&key[cat.configOffset], cat.keys(), &key[cat2.configOffset]);
			std::fill_n(&key[cat.configOffset], cat.keys(), 0);
		}
		else if(i)
		{
			//logMsg("clearing player %d map", i);
			auto cat2 = categories()[startCategory+i];
			std::fill_n(&key[cat2.configOffset], cat2.keys(), 0);
		}
	}
}

}

void EmuApp::updateKeyboardMapping()
{
	defaultVController().updateKeyboardMapping();
}

void EmuApp::toggleKeyboard()
{
	defaultVController().toggleKeyboard();
}

void EmuApp::setDisabledInputKeys(std::span<const unsigned> keys)
{
	auto &vController = inputManager.vController;
	vController.setDisabledInputKeys(keys);
	if(!vController.hasWindow())
		return;
	vController.place();
	system().clearInputBuffers(viewController().inputView);
}

void EmuApp::unsetDisabledInputKeys()
{
	setDisabledInputKeys({});
}

bool KeyConfig::operator ==(KeyConfig const& rhs) const
{
	return name == rhs.name;
}

KeyConfig::Key *KeyConfig::key(const KeyCategory &category)
{
	assert(category.configOffset + category.keys() <= MAX_KEY_CONFIG_KEYS);
	return &key_[category.configOffset];
}

const KeyConfig::Key *KeyConfig::key(const KeyCategory &category) const
{
	assert(category.configOffset + category.keys() <= MAX_KEY_CONFIG_KEYS);
	return &key_[category.configOffset];
}

void KeyConfig::unbindCategory(const KeyCategory &category)
{
	std::fill_n(key(category), category.keys(), 0);
}

bool InputDeviceSavedConfig::matchesDevice(const Input::Device &dev) const
{
	//logMsg("checking against device %s,%d", name, devId);
	return dev.enumId() == enumId && dev.name() == name;
}

}

namespace EmuEx::Controls
{

[[gnu::weak]] void transposeKeysForPlayer(KeyConfig::KeyArray &key, int player) {}

}
