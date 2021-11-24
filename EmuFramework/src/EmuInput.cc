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

struct RelPtr  // for Android trackball
{
	int x = 0, y = 0;
	unsigned xAction = 0, yAction = 0;
};
static RelPtr relPtr{};

std::list<InputDeviceSavedConfig> savedInputDevList{};
std::list<KeyConfig> customKeyConfig{};

static void processRelPtr(EmuApp &app, Input::Event e)
{
	using namespace IG;
	if(relPtr.x != 0 && sign(relPtr.x) != sign(e.pos().x))
	{
		//logMsg("reversed trackball X direction");
		relPtr.x = e.pos().x;
		EmuSystem::handleInputAction(&app, Input::Action::RELEASED, relPtr.xAction);
	}
	else
		relPtr.x += e.pos().x;

	if(e.pos().x)
	{
		relPtr.xAction = EmuSystem::translateInputAction(e.pos().x > 0 ? EmuControls::systemKeyMapStart+1 : EmuControls::systemKeyMapStart+3);
		EmuSystem::handleInputAction(&app, Input::Action::PUSHED, relPtr.xAction);
	}

	if(relPtr.y != 0 && sign(relPtr.y) != sign(e.pos().y))
	{
		//logMsg("reversed trackball Y direction");
		relPtr.y = e.pos().y;
		EmuSystem::handleInputAction(&app, Input::Action::RELEASED, relPtr.yAction);
	}
	else
		relPtr.y += e.pos().y;

	if(e.pos().y)
	{
		relPtr.yAction = EmuSystem::translateInputAction(e.pos().y > 0 ? EmuControls::systemKeyMapStart+2 : EmuControls::systemKeyMapStart);
		EmuSystem::handleInputAction(&app, Input::Action::PUSHED, relPtr.yAction);
	}

	//logMsg("trackball event %d,%d, rel ptr %d,%d", e.x, e.y, relPtr.x, relPtr.y);
}

void TurboInput::update(EmuApp *app)
{
	static const unsigned turboFrames = 4;

	for(auto e : activeAction)
	{
		if(e.action)
		{
			if(clock == 0)
			{
				//logMsg("turbo push for player %d, action %d", e.player, e.action);
				EmuSystem::handleInputAction(app, Input::Action::PUSHED, e.action);
			}
			else if(clock == turboFrames/2)
			{
				//logMsg("turbo release for player %d, action %d", e.player, e.action);
				EmuSystem::handleInputAction(app, Input::Action::RELEASED, e.action);
			}
		}
	}
	clock++;
	if(clock == turboFrames) clock = 0;
}

void commonUpdateInput()
{
#if 0
	auto applyRelPointerDecel =
		[](int val)
		{
			return std::max(std::abs(val) - (int)optionRelPointerDecel, 0) * IG::sign(val);
		};

	if(relPtr.x)
	{
		relPtr.x = applyRelPointerDecel(relPtr.x);
		if(!relPtr.x)
			EmuSystem::handleInputAction(Input::RELEASED, relPtr.xAction);
	}
	if(relPtr.y)
	{
		relPtr.y = applyRelPointerDecel(relPtr.y);
		if(!relPtr.y)
			EmuSystem::handleInputAction(Input::RELEASED, relPtr.yAction);
	}
#endif
}

void EmuApp::updateInputDevices(Base::ApplicationContext ctx)
{
	for(auto &devPtr : ctx.inputDevices())
	{
		logMsg("input device:%s, id:%d, map:%d", devPtr->name().data(), devPtr->enumId(), (int)devPtr->map());
		auto &appData = devPtr->makeAppData<InputDeviceData>(*devPtr, savedInputDevList);
	}
	vController.setPhysicalControlsPresent(ctx.keyInputIsPresent());
	onUpdateInputDevices_.callCopySafe();
}

InputDeviceData::InputDeviceData(Input::Device &dev, std::list<InputDeviceSavedConfig> &savedInputDevList):
	devConf{dev},
	displayName{makeDisplayName(dev.name(), dev.enumId())}
{
	dev.setJoystickAxisAsDpadBits(Input::Device::AXIS_BITS_STICK_1 | Input::Device::AXIS_BITS_HAT);
	for(auto &saved : savedInputDevList)
	{
		if(saved.matchesDevice(dev))
		{
			logMsg("has saved config");
			devConf.setSavedConf(&saved, false);
		}
	}
	buildKeyMap(dev);
}

void InputDeviceData::buildKeyMap(const Input::Device &d)
{
	auto totalKeys = Input::Event::mapNumKeys(d.map());
	if(!totalKeys || !devConf.isEnabled()) [[unlikely]]
		return;
	logMsg("allocating key mapping for:%s with player:%d", d.name().data(), devConf.player()+1);
	actionTable.resize(totalKeys);
	KeyConfig::KeyArray key = devConf.keyConf().key();
	if(devConf.player() != InputDeviceConfig::PLAYER_MULTI)
	{
		EmuControls::transposeKeysForPlayer(key, devConf.player());
	}
	iterateTimes(MAX_KEY_CONFIG_KEYS, k)
	{
		//logMsg("mapping key %d to %u %s", k, key, Input::buttonName(devConf.dev->map, key[k]));
		assert(key[k] < totalKeys);
		auto &group = actionTable[key[k]];
		auto slot = IG::find_if(group, [](auto &a){ return a == 0; });
		if(slot != group.end())
			*slot = k+1; // add 1 to avoid 0 value (considered unmapped)
	}
}

std::string InputDeviceData::makeDisplayName(std::string_view name, unsigned id)
{
	if(id)
	{
		return fmt::format("{} #{}", name, id + 1);
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
			#if defined __ANDROID__ || defined CONFIG_BASE_X11
			unsigned confs = 0;
			auto conf = defaultConfigsForDevice(dev, confs);
			iterateTimes(confs, i)
			{
				// Look for the first config to match the device subtype
				if(dev.subtype() == conf[i].devSubtype)
				{
					return conf[i];
				}
			}
			#endif
			return defaultConfigsForDevice(dev)[0];
		}
	}
}

const KeyConfig *KeyConfig::defaultConfigsForInputMap(Input::Map map, unsigned &size)
{
	switch(map)
	{
		default: return nullptr;
		case Input::Map::SYSTEM:
			size = EmuControls::defaultKeyProfiles;
			return EmuControls::defaultKeyProfile;
		#ifdef CONFIG_BLUETOOTH
		case Input::Map::WIIMOTE:
			size = EmuControls::defaultWiimoteProfiles;
			return EmuControls::defaultWiimoteProfile;
		case Input::Map::WII_CC:
			size = EmuControls::defaultWiiCCProfiles;
			return EmuControls::defaultWiiCCProfile;
		case Input::Map::ICONTROLPAD:
			size = EmuControls::defaultIControlPadProfiles;
			return EmuControls::defaultIControlPadProfile;
		case Input::Map::ZEEMOTE:
			size = EmuControls::defaultZeemoteProfiles;
			return EmuControls::defaultZeemoteProfile;
		#endif
		#ifdef CONFIG_BLUETOOTH_SERVER
		case Input::Map::PS3PAD:
			size = EmuControls::defaultPS3Profiles;
			return EmuControls::defaultPS3Profile;
		#endif
		#ifdef CONFIG_INPUT_ICADE
		case Input::Map::ICADE:
			size = EmuControls::defaultICadeProfiles;
			return EmuControls::defaultICadeProfile;
		#endif
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case Input::Map::APPLE_GAME_CONTROLLER:
			size = EmuControls::defaultAppleGCProfiles;
			return EmuControls::defaultAppleGCProfile;
		#endif
	}
}

const KeyConfig *KeyConfig::defaultConfigsForDevice(const Input::Device &dev, unsigned &size)
{
	auto conf = defaultConfigsForInputMap(dev.map(), size);
	if(!conf)
	{
		bug_unreachable("device map:%d missing default configs", (int)dev.map());
		return nullptr;
	}
	return conf;
}

const KeyConfig *KeyConfig::defaultConfigsForDevice(const Input::Device &dev)
{
	unsigned size;
	return defaultConfigsForDevice(dev, size);
}

// InputDeviceConfig

static IG::StaticString<16> uniqueCustomConfigName()
{
	iterateTimes(99, i) // Try up to "Custom 99"
	{
		auto name = IG::format<IG::StaticString<16>>("Custom {}", i+1);
		// Check if this name is free
		logMsg("checking:%s", name.data());
		bool exists{};
		for(auto &e : customKeyConfig)
		{
			logMsg("against:%s", e.name.data());
			if(e.name == name)
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

void InputDeviceConfig::deleteConf()
{
	if(savedConf)
	{
		logMsg("removing device config for %s", savedConf->name.data());
		auto removed = IG::eraseFirst(savedInputDevList, *savedConf);
		assert(removed);
		savedConf = nullptr;
	}
}

#ifdef CONFIG_INPUT_ICADE
bool InputDeviceConfig::setICadeMode(bool on)
{
	// delete device's config since its properties will change with iCade mode switch
	deleteConf();
	dev->setICadeMode(on);
	buildKeyMap();
	save();
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

void InputDeviceConfig::setKeyConf(const KeyConfig &kConf)
{
	save();
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

KeyConfig *InputDeviceConfig::mutableKeyConf()
{
	auto currConf = &keyConf();
	//logMsg("curr key config %p", currConf);
	for(auto &e : customKeyConfig)
	{
		//logMsg("checking key config %p", &e);
		if(&e == currConf)
		{
			return &e;
		}
	}
	return nullptr;
}

KeyConfig *InputDeviceConfig::makeMutableKeyConf(EmuApp &app)
{
	auto conf = mutableKeyConf();
	if(!conf)
	{
		logMsg("current config not mutable, creating one");
		auto name = uniqueCustomConfigName();
		conf = setKeyConfCopiedFromExisting(name);
		app.postMessage(3, false, fmt::format("Automatically created profile: {}", conf->name));
	}
	return conf;
}

KeyConfig *InputDeviceConfig::setKeyConfCopiedFromExisting(std::string_view name)
{
	auto &newConf = customKeyConfig.emplace_back(keyConf());
	newConf.name = name;
	setKeyConf(newConf);
	return &newConf;
}

void InputDeviceConfig::save()
{
	if(!savedConf)
	{
		savedConf = &savedInputDevList.emplace_back();
		logMsg("allocated new device config, %d total", (int)savedInputDevList.size());
	}
	savedConf->player = player_;
	savedConf->enabled = enabled;
	savedConf->enumId = dev->enumId();
	savedConf->joystickAxisAsDpadBits = dev->joystickAxisAsDpadBits();
	#ifdef CONFIG_INPUT_ICADE
	savedConf->iCadeMode = dev->iCadeMode();
	#endif
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

namespace EmuControls
{

void generic2PlayerTranspose(KeyConfig::KeyArray &key, unsigned player, unsigned startCategory)
{
	if(player == 0)
	{
		// clear P2 joystick keys
		std::fill_n(&key[category[startCategory+1].configOffset], category[startCategory+1].keys, 0);
	}
	else
	{
		// transpose joystick keys
		std::copy_n(&key[category[startCategory].configOffset], category[startCategory].keys, &key[category[startCategory+1].configOffset]);
		std::fill_n(&key[category[startCategory].configOffset], category[startCategory].keys, 0);
	}
}

void genericMultiplayerTranspose(KeyConfig::KeyArray &key, unsigned player, unsigned startCategory)
{
	iterateTimes(EmuSystem::maxPlayers, i)
	{
		if(player && i == player)
		{
			//logMsg("moving to player %d map", i);
			std::copy_n(&key[category[startCategory].configOffset], category[startCategory].keys, &key[category[i+startCategory].configOffset]);
			std::fill_n(&key[category[startCategory].configOffset], category[startCategory].keys, 0);
		}
		else if(i)
		{
			//logMsg("clearing player %d map", i);
			std::fill_n(&key[category[i+startCategory].configOffset], category[i+startCategory].keys, 0);
		}
	}
}

}

void EmuApp::applyEnabledFaceButtons(std::span<const std::pair<int, bool>> applyEnableMap)
{
	if constexpr(Config::EmuFramework::VCONTROLS_GAMEPAD)
	{
		auto &vController = defaultVController();
		auto &btnGroup = vController.gamePad().faceButtons();
		for(auto [idx, enabled] : applyEnableMap)
		{
			btnGroup.buttons()[idx].setEnabled(enabled);
		}
		if(!vController.hasWindow())
			return;
		vController.place();
		EmuSystem::clearInputBuffers(emuViewController->inputView());
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

void EmuApp::updateVControllerMapping()
{
	defaultVController().updateMapping();
}

void TurboInput::addEvent(unsigned action)
{
	Action *slot = IG::find_if(activeAction, [](Action a){ return a == 0; });
	if(slot != activeAction.end())
	{
		slot->action = action;
		logMsg("added turbo event action %d", action);
	}
}

void TurboInput::removeEvent(unsigned action)
{
	for(auto &e : activeAction)
	{
		if(e.action == action)
		{
			e.action = 0;
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
	assert(category.configOffset + category.keys <= MAX_KEY_CONFIG_KEYS);
	return &key_[category.configOffset];
}

const KeyConfig::Key *KeyConfig::key(const KeyCategory &category) const
{
	assert(category.configOffset + category.keys <= MAX_KEY_CONFIG_KEYS);
	return &key_[category.configOffset];
}

void KeyConfig::unbindCategory(const KeyCategory &category)
{
	std::fill_n(key(category), category.keys, 0);
}
