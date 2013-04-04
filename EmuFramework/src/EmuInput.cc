/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <EmuSystem.hh>
#include <EmuInput.hh>
#include <EmuOptions.hh>
#include <InputManagerView.hh>

#ifdef INPUT_SUPPORTS_POINTER
uint pointerInputPlayer = 0;
#endif

struct RelPtr  // for Android trackball
{
	int x, y;
	uint xAction, yAction;
};
static RelPtr relPtr = { 0 };

TurboInput turboActions;
extern ViewStack viewStack;
extern InputManagerDeviceView imdMenu;
extern InputManagerView imMenu;
uint inputDevConfs = 0;
InputDeviceConfig inputDevConf[Input::MAX_DEVS];
StaticDLList<InputDeviceSavedConfig, MAX_SAVED_INPUT_DEVICES> savedInputDevList;
KeyMapping keyMapping;
StaticDLList<KeyConfig, MAX_CUSTOM_KEY_CONFIGS> customKeyConfig;
bool physicalControlsPresent = 0;

#ifdef INPUT_SUPPORTS_POINTER
void processRelPtr(const Input::Event &e)
{
	using namespace IG;
	if(relPtr.x != 0 && signOf(relPtr.x) != signOf(e.x))
	{
		//logMsg("reversed trackball X direction");
		relPtr.x = e.x;
		EmuSystem::handleInputAction(Input::RELEASED, relPtr.xAction);
	}
	else
		relPtr.x += e.x;

	if(e.x)
	{
		relPtr.xAction = EmuSystem::translateInputAction(e.x > 0 ? EmuControls::systemKeyMapStart+1 : EmuControls::systemKeyMapStart+3);
		EmuSystem::handleInputAction(Input::PUSHED, relPtr.xAction);
	}

	if(relPtr.y != 0 && signOf(relPtr.y) != signOf(e.y))
	{
		//logMsg("reversed trackball Y direction");
		relPtr.y = e.y;
		EmuSystem::handleInputAction(Input::RELEASED, relPtr.yAction);
	}
	else
		relPtr.y += e.y;

	if(e.y)
	{
		relPtr.yAction = EmuSystem::translateInputAction(e.y > 0 ? EmuControls::systemKeyMapStart+2 : EmuControls::systemKeyMapStart);
		EmuSystem::handleInputAction(Input::PUSHED, relPtr.yAction);
	}

	//logMsg("trackball event %d,%d, rel ptr %d,%d", e.x, e.y, relPtr.x, relPtr.y);
}
#endif

void commonInitInput()
{
	mem_zero(relPtr);
	mem_zero(turboActions);
}

void commonUpdateInput()
{
	using namespace IG;
	static const uint turboFrames = 4;
	static uint turboClock = 0;

	forEachInArray(turboActions.activeAction, e)
	{
		if(e->action)
		{
			if(turboClock == 0)
			{
				//logMsg("turbo push for player %d, action %d", e->player, e->action);
				EmuSystem::handleInputAction(Input::PUSHED, e->action);
			}
			else if(turboClock == turboFrames/2)
			{
				//logMsg("turbo release for player %d, action %d", e->player, e->action);
				EmuSystem::handleInputAction(Input::RELEASED, e->action);
			}
		}
	}
	turboClock++;
	if(turboClock == turboFrames) turboClock = 0;

#ifdef INPUT_SUPPORTS_POINTER
	if(relPtr.x)
	{
		relPtr.x = clipToZeroSigned(relPtr.x, (int)optionRelPointerDecel * -signOf(relPtr.x));
		if(!relPtr.x)
			EmuSystem::handleInputAction(Input::RELEASED, relPtr.xAction);
	}
	if(relPtr.y)
	{
		relPtr.y = clipToZeroSigned(relPtr.y, (int)optionRelPointerDecel * -signOf(relPtr.y));
		if(!relPtr.y)
			EmuSystem::handleInputAction(Input::RELEASED, relPtr.yAction);
	}
#endif
}

bool isMenuDismissKey(const Input::Event &e)
{
	using namespace Input;
	#ifdef INPUT_SUPPORTS_KEYBOARD
	static const auto dismissKey = Config::envIsWebOS1 ? Keycode::RCTRL : Keycode::MENU;
	#endif
	switch(e.map)
	{
		#ifdef CONFIG_BLUETOOTH
		case Event::MAP_WIIMOTE: return e.button == Wiimote::HOME;
		case Event::MAP_WII_CC: return e.button == WiiCC::HOME;
		case Event::MAP_ICONTROLPAD: return e.button == iControlPad::Y;
		case Event::MAP_ZEEMOTE: return e.button == Zeemote::POWER;
		#endif
		#ifdef CONFIG_INPUT_ICADE
		case Event::MAP_ICADE: return e.button == ICade::G;
		#endif
		#if defined(CONFIG_BASE_PS3)
		case Event::MAP_PS3PAD:
			return e.button == PS3::TRIANGLE || e.button == PS3::L2;
		#endif
		#ifdef INPUT_SUPPORTS_KEYBOARD
		case Event::MAP_KEYBOARD:
			#ifdef CONFIG_BASE_ANDROID
			switch(e.device->subtype)
			{
				case Device::SUBTYPE_PS3_CONTROLLER:
					return e.button == PS3::PS;
			}
			#endif
			return e.button == dismissKey;
		#endif
	}
	return 0;
}

void updateInputDevices()
{
	using namespace Input;
	int i = 0;
	forEachInDLList(&devList, e)
	{
		logMsg("input device %d: name: %s, id: %d, map: %d", i, e.name(), e.devId, e.map());
		inputDevConf[i].dev = &e;
		inputDevConf[i].reset();
		forEachInDLList(&savedInputDevList, saved)
		{
			if(saved.matchesDevice(e))
			{
				logMsg("has saved config");
				inputDevConf[i].setSavedConf(&saved);
			}
		}
		i++;
	}
	inputDevConfs = i;

	physicalControlsPresent = keyInputIsPresent();
	if(physicalControlsPresent)
	{
		logMsg("Physical controls are present");
	}

	if(viewStack.contains(&imMenu))
	{
		if(View::modalView)
			View::removeModalView();
		viewStack.popToRoot();
		imMenu.init(0);
		viewStack.pushAndShow(&imMenu);
	}

	keyMapping.buildAll();
}

// KeyConfig

const KeyConfig &KeyConfig::defaultConfigForDevice(const Input::Device &dev)
{
	switch(dev.map())
	{
		case Input::Event::MAP_KEYBOARD:
		{
			#ifdef CONFIG_BASE_ANDROID
			uint confs = 0;
			auto conf = defaultConfigsForDevice(dev, confs);
			iterateTimes(confs, i)
			{
				// Look for the first config to match the device subtype
				if(dev.subtype == conf[i].devSubtype)
				{
					return conf[i];
				}
			}
			#endif
			return defaultConfigsForDevice(dev)[0];
		}
	}
	return defaultConfigsForDevice(dev)[0];
}

const KeyConfig *KeyConfig::defaultConfigsForInputMap(uint map, uint &size)
{
	switch(map)
	{
		#ifdef INPUT_SUPPORTS_KEYBOARD
		case Input::Event::MAP_KEYBOARD:
			size = EmuControls::defaultKeyProfiles;
			return EmuControls::defaultKeyProfile;
		#endif
		#ifdef CONFIG_BLUETOOTH
		case Input::Event::MAP_WIIMOTE:
			size = EmuControls::defaultWiimoteProfiles;
			return EmuControls::defaultWiimoteProfile;
		case Input::Event::MAP_WII_CC:
			size = EmuControls::defaultWiiCCProfiles;
			return EmuControls::defaultWiiCCProfile;
		case Input::Event::MAP_ICONTROLPAD:
			size = EmuControls::defaultIControlPadProfiles;
			return EmuControls::defaultIControlPadProfile;
		case Input::Event::MAP_ZEEMOTE:
			size = EmuControls::defaultZeemoteProfiles;
			return EmuControls::defaultZeemoteProfile;
		#endif
//		#ifdef CONFIG_BASE_PS3
//		case MAP_PS3PAD: bug_exit("TODO");
//		#endif
		#ifdef CONFIG_INPUT_ICADE
		case Input::Event::MAP_ICADE:
			size = EmuControls::defaultICadeProfiles;
			return EmuControls::defaultICadeProfile;
		#endif
	}
	return nullptr;
}

const KeyConfig *KeyConfig::defaultConfigsForDevice(const Input::Device &dev, uint &size)
{
	auto conf = defaultConfigsForInputMap(dev.map(), size);
	if(!conf)
	{
		bug_exit("device type %d missing default configs", dev.map());
		return nullptr;
	}
	return conf;
}

const KeyConfig *KeyConfig::defaultConfigsForDevice(const Input::Device &dev)
{
	uint size;
	return defaultConfigsForDevice(dev, size);
}

// InputDeviceConfig

void InputDeviceConfig::reset()
{
	assert(dev);
	savedConf = nullptr;
	player = dev->devId < EmuSystem::maxPlayers ? dev->devId : 0;
	enabled = 1;
}

void InputDeviceConfig::deleteConf()
{
	if(savedConf)
	{
		logMsg("removing device config for %s", savedConf->name);
		auto removed = savedInputDevList.remove(*savedConf);
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

bool InputDeviceConfig::mapJoystickAxis1ToDpad()
{
	return dev->mapJoystickAxis1ToDpad;
}

void InputDeviceConfig::setMapJoystickAxis1ToDpad(bool on)
{
	dev->mapJoystickAxis1ToDpad = on;
}

const KeyConfig &InputDeviceConfig::keyConf()
{
	if(savedConf && savedConf->keyConf)
	{
		//logMsg("has saved config %p", savedConf->keyConf_);
		return *savedConf->keyConf;
	}
	return KeyConfig::defaultConfigForDevice(*dev);
}

bool InputDeviceConfig::setKeyConf(const KeyConfig &kConf)
{
	save();
	if(!savedConf)
	{
		logErr("can't set key config");
		return 0;
	}
	savedConf->keyConf = &kConf;
	return 1;
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
	forEachInDLList(&customKeyConfig, e)
	{
		//logMsg("checking key config %p", &e);
		if(&e == currConf)
		{
			return &e;
		}
	}
	return nullptr;
}

KeyConfig *InputDeviceConfig::setKeyConfCopiedFromExisting(const char *name)
{
	if(!customKeyConfig.addToEnd())
	{
		bug_exit("should no be called with full key config list");
		return nullptr;
	}
	auto newConf = customKeyConfig.last();
	*newConf = keyConf();
	string_copy(newConf->name, name);
	if(!setKeyConf(*newConf))
	{
		// No space left for new device settings
		customKeyConfig.removeLast();
		return nullptr;
	}
	return newConf;
}

void InputDeviceConfig::save()
{
	if(!savedConf)
	{
		if(!savedInputDevList.addToEnd())
		{
			logWarn("no more space for device configs");
			return;
		}
		logMsg("allocated new device config, %d total", savedInputDevList.size);
		savedConf = savedInputDevList.last();
		*savedConf = InputDeviceSavedConfig();
	}
	savedConf->player = player;
	savedConf->enabled = enabled;
	savedConf->devId = dev->devId;
	savedConf->mapJoystickAxis1ToDpad = dev->mapJoystickAxis1ToDpad;
	#ifdef CONFIG_INPUT_ICADE
	savedConf->iCadeMode = dev->iCadeMode();
	#endif
	string_copy(savedConf->name, dev->name());
}

void InputDeviceConfig::setSavedConf(InputDeviceSavedConfig *savedConf)
{
	var_selfs(savedConf);
	if(savedConf)
	{
		player = savedConf->player;
		enabled = savedConf->enabled;
		dev->mapJoystickAxis1ToDpad = savedConf->mapJoystickAxis1ToDpad;
		#ifdef CONFIG_INPUT_ICADE
		dev->setICadeMode(savedConf->iCadeMode);
		#endif
	}
}

// KeyMapping

void KeyMapping::buildAll()
{
	assert((int)inputDevConfs == Input::devList.size);
	// calculate & allocate complete map including all devices
	{
		uint totalKeys = 0;
		forEachInDLList(&Input::devList, e)
		{
			totalKeys += Input::Event::mapNumKeys(e.map());
		}
		if(unlikely(!totalKeys))
		{
			logMsg("no keys in mapping");
			if(inputDevActionTablePtr[0])
			{
				mem_free(inputDevActionTablePtr[0]);
				inputDevActionTablePtr[0] = nullptr;
			}
			return;
		}
		logMsg("allocating key mapping with %d keys", totalKeys);
		inputDevActionTablePtr[0] = (ActionGroup*)mem_realloc(inputDevActionTablePtr[0], totalKeys * sizeof(ActionGroup));
		mem_zero(inputDevActionTablePtr[0], totalKeys * sizeof(ActionGroup));
	}
	uint totalKeys = 0;
	int i = 0;
	forEachInDLList(&Input::devList, e)
	{
		if(i)
		{
			// [0] is the base pointer to the allocated map, subsequent elements
			// point to an offset within it
			inputDevActionTablePtr[i] = &inputDevActionTablePtr[0][totalKeys];
		}
		auto mapKeys = Input::Event::mapNumKeys(e.map());
		totalKeys += mapKeys;
		auto actionGroup = inputDevActionTablePtr[i];
		if(!inputDevConf[i].enabled)
		{
			i++;
			continue;
		}
		KeyConfig::KeyArray key;
		memcpy(key, inputDevConf[i].keyConf().key(), sizeof(key));
		if(inputDevConf[i].player != InputDeviceConfig::PLAYER_MULTI)
		{
			logMsg("transposing keys for player %d", inputDevConf[i].player+1);
			EmuControls::transposeKeysForPlayer(key, inputDevConf[i].player);
		}
		iterateTimes(MAX_KEY_CONFIG_KEYS, k)
		{
			//logMsg("mapping key %d to %u %s", k, key, Input::buttonName(inputDevConf[i].dev->map, key[k]));
			assert(key[k] < Input::Event::mapNumKeys(e.map()));
			auto slot = IG::mem_findFirstZeroValue(actionGroup[key[k]]);
			if(slot)
				*slot = k+1; // add 1 to avoid 0 value (considered unmapped)
		}

		i++;
	}
}

namespace EmuControls
{

void generic2PlayerTranspose(KeyConfig::KeyArray &key, uint player, uint startCategory)
{
	if(player == 0)
	{
		// clear P2 joystick keys
		mem_zero(&key[category[startCategory+1].configOffset], category[startCategory+1].keys * sizeof(KeyConfig::Key));
	}
	else
	{
		// transpose joystick keys
		memcpy(&key[category[startCategory+1].configOffset], &key[category[startCategory].configOffset], category[startCategory].keys * sizeof(KeyConfig::Key));
		mem_zero(&key[category[startCategory].configOffset], category[startCategory].keys * sizeof(KeyConfig::Key));
	}
}

void genericMultiplayerTranspose(KeyConfig::KeyArray &key, uint player, uint startCategory)
{
	iterateTimes(EmuSystem::maxPlayers, i)
	{
		if(player && i == player)
		{
			//logMsg("moving to player %d map", i);
			memcpy(&key[category[i+startCategory].configOffset], &key[category[startCategory].configOffset], category[startCategory].keys * sizeof(KeyConfig::Key));
			mem_zero(&key[category[startCategory].configOffset], category[startCategory].keys * sizeof(KeyConfig::Key));
		}
		else if(i)
		{
			//logMsg("clearing player %d map", i);
			mem_zero(&key[category[i+startCategory].configOffset], category[i+startCategory].keys * sizeof(KeyConfig::Key));
		}
	}
}

}
