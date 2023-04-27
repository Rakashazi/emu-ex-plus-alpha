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
#include <emuframework/InputManagerView.hh>
#include "privateInput.hh"
#include "WindowData.hh"
#include "EmuOptions.hh"
#include <imagine/base/ApplicationContext.hh>
#include <imagine/util/algorithm.h>
#include <imagine/util/math/int.hh>
#include <imagine/util/format.hh>
#include <imagine/gfx/Renderer.hh>
#include <cstdlib>

namespace EmuEx
{

void TurboInput::update(EmuApp &app)
{
	static const int turboFrames = 4;

	for(auto e : activeAction)
	{
		if(e)
		{
			if(clock == 0)
			{
				//logMsg("turbo push for player %d, action %d", e.player, e.action);
				app.system().handleInputAction(&app, {e, Input::Action::PUSHED});
			}
			else if(clock == turboFrames/2)
			{
				//logMsg("turbo release for player %d, action %d", e.player, e.action);
				app.system().handleInputAction(&app, {e, Input::Action::RELEASED});
			}
		}
	}
	clock++;
	if(clock == turboFrames) clock = 0;
}

void EmuApp::updateInputDevices(IG::ApplicationContext ctx)
{
	for(auto &devPtr : ctx.inputDevices())
	{
		logMsg("input device:%s, id:%d, map:%d", devPtr->name().data(), devPtr->enumId(), (int)devPtr->map());
		auto &appData = devPtr->makeAppData<InputDeviceData>(*devPtr, savedInputDevs);
	}
	vController.setPhysicalControlsPresent(ctx.keyInputIsPresent());
	onUpdateInputDevices_.callCopySafe();
}

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

void EmuApp::setOnUpdateInputDevices(DelegateFunc<void ()> del)
{
	if(del)
	{
		assert(!onUpdateInputDevices_);
	}
	onUpdateInputDevices_ = del;
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

// InputDeviceConfig

static IG::StaticString<16> uniqueCustomConfigName(KeyConfigContainer &customKeyConfigs)
{
	for(auto i : iotaCount(100)) // Try up to "Custom 99"
	{
		auto name = IG::format<IG::StaticString<16>>("Custom {}", i+1);
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

void TurboInput::addEvent(unsigned action)
{
	Action *slot = std::ranges::find_if(activeAction, [](Action a){ return a == 0; });
	if(slot != activeAction.end())
	{
		*slot = action;
		logMsg("added turbo event action %d", action);
	}
}

void TurboInput::removeEvent(unsigned action)
{
	for(auto &e : activeAction)
	{
		if(e == action)
		{
			e = 0;
			logMsg("removed turbo event action %d", action);
		}
	}
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
