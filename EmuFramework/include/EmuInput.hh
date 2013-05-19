#pragma once

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

#include <input/Input.hh>
#ifdef CONFIG_BLUETOOTH
#include <bluetooth/BluetoothInputDevScanner.hh>
#endif
#include <TurboInput.hh>
#include <EmuSystem.hh>
#include <inGameActionKeys.hh>

struct KeyCategory
{
	template <size_t S>
	constexpr KeyCategory(const char *name, const char *(&keyName)[S],
			uint configOffset, bool isMultiplayer = 0) :
	name(name), keyName(keyName), keys(S), configOffset(configOffset), isMultiplayer(isMultiplayer) { }

	const char *name;
	const char **keyName;
	uint keys;
	uint configOffset;
	bool isMultiplayer; // category appears when one input device is assigned multiple players
};

#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
extern uint pointerInputPlayer;
#endif

static const int guiKeyIdxLoadGame = 0;
static const int guiKeyIdxMenu = 1;
static const int guiKeyIdxSaveState = 2;
static const int guiKeyIdxLoadState = 3;
static const int guiKeyIdxDecStateSlot = 4;
static const int guiKeyIdxIncStateSlot = 5;
static const int guiKeyIdxFastForward = 6;
static const int guiKeyIdxGameScreenshot = 7;
static const int guiKeyIdxExit = 8;

void processRelPtr(const Input::Event &e);
void commonInitInput();
void commonUpdateInput();
extern TurboInput turboActions;

static constexpr uint MAX_KEY_CONFIG_KEYS = EmuControls::systemTotalKeys;
static constexpr uint MAX_DEFAULT_KEY_CONFIGS_PER_TYPE = 8;
static constexpr uint MAX_CUSTOM_KEY_CONFIGS = 10;
static constexpr uint MAX_KEY_CONFIG_NAME_SIZE = 80;

struct KeyConfig
{
	uint map;
	uint devSubtype;
	char name[MAX_KEY_CONFIG_NAME_SIZE];
	typedef Input::Key Key;
	typedef Key KeyArray[MAX_KEY_CONFIG_KEYS];
	KeyArray key_;

	bool operator ==(KeyConfig const& rhs) const
	{
		return string_equal(name, rhs.name);
	}

	Key *key(const KeyCategory &category)
	{
		assert(category.configOffset + category.keys <= MAX_KEY_CONFIG_KEYS);
		return &key_[category.configOffset];
	}

	const Key *key(const KeyCategory &category) const
	{
		assert(category.configOffset + category.keys <= MAX_KEY_CONFIG_KEYS);
		return &key_[category.configOffset];
	}

	const KeyArray &key() const
	{
		return key_;
	}

	KeyArray &key()
	{
		return key_;
	}

	void unbindCategory(const KeyCategory &category)
	{
		mem_zero(key(category), category.keys * sizeof(Key));
	}

	static const KeyConfig *defaultConfigsForInputMap(uint map, uint &size);
	static const KeyConfig &defaultConfigForDevice(const Input::Device &dev);
	static const KeyConfig *defaultConfigsForDevice(const Input::Device &dev, uint &size);
	static const KeyConfig *defaultConfigsForDevice(const Input::Device &dev);
};

extern StaticDLList<KeyConfig, MAX_CUSTOM_KEY_CONFIGS> customKeyConfig;

static bool customKeyConfigsContainName(const char *name)
{
	forEachInDLList(&customKeyConfig, e)
	{
		if(string_equal(e.name, name))
			return 1;
	}
	return 0;
}

static constexpr uint MAX_SAVED_INPUT_DEVICES = Input::MAX_DEVS;
static constexpr uint MAX_INPUT_DEVICE_NAME_SIZE = 64;

struct InputDeviceSavedConfig
{
	constexpr InputDeviceSavedConfig() { }
	const KeyConfig *keyConf = nullptr;
	uint devId = 0;
	uint8 player = 0;
	bool enabled = 1;
	bool mapJoystickAxis1ToDpad = 0;
	#ifdef CONFIG_INPUT_ICADE
		bool iCadeMode = 0;
	#endif
	char name[MAX_INPUT_DEVICE_NAME_SIZE] {0};

	bool operator ==(InputDeviceSavedConfig const& rhs) const
	{
		return devId == rhs.devId && string_equal(name, rhs.name);
	}

	bool matchesDevice(const Input::Device &dev)
	{
		//logMsg("checking against device %s,%d", name, devId);
		return dev.devId == devId &&
			string_equal(dev.name(), name);
	}
};

extern StaticDLList<InputDeviceSavedConfig, MAX_SAVED_INPUT_DEVICES> savedInputDevList;

struct InputDeviceConfig
{
	constexpr InputDeviceConfig() { }
	static constexpr uint PLAYER_MULTI = 0xFF;
	uint8 player = 0;
	bool enabled = 1;
	Input::Device *dev = nullptr;
	InputDeviceSavedConfig *savedConf = nullptr;

	void reset();
	void deleteConf();
	#ifdef CONFIG_INPUT_ICADE
	bool setICadeMode(bool on);
	bool iCadeMode();
	#endif
	bool mapJoystickAxis1ToDpad();
	void setMapJoystickAxis1ToDpad(bool on);
	const KeyConfig &keyConf();
	bool setKeyConf(const KeyConfig &kConf);
	void setDefaultKeyConf();
	KeyConfig *mutableKeyConf();
	KeyConfig *setKeyConfCopiedFromExisting(const char *name);
	void save();
	void setSavedConf(InputDeviceSavedConfig *savedConf);
};

extern uint inputDevConfs;
extern InputDeviceConfig inputDevConf[Input::MAX_DEVS];

struct KeyMapping
{
	constexpr KeyMapping() { }
	static constexpr uint maxKeyActions = 4;
	typedef uint8 Action;
	typedef Action ActionGroup[maxKeyActions];
	ActionGroup *inputDevActionTablePtr[Input::MAX_DEVS] {nullptr};
	void buildAll();
};

extern KeyMapping keyMapping;

void updateInputDevices();
extern bool physicalControlsPresent;

namespace EmuControls
{

// Defined in the emulation module
extern const KeyCategory category[categories];

#ifdef INPUT_SUPPORTS_KEYBOARD
	extern const KeyConfig defaultKeyProfile[];
	extern const uint defaultKeyProfiles;
#endif

extern const KeyConfig defaultWiimoteProfile[];
extern const uint defaultWiimoteProfiles;
extern const KeyConfig defaultWiiCCProfile[];
extern const uint defaultWiiCCProfiles;
extern const KeyConfig defaultIControlPadProfile[];
extern const uint defaultIControlPadProfiles;
extern const KeyConfig defaultICadeProfile[];
extern const uint defaultICadeProfiles;
extern const KeyConfig defaultZeemoteProfile[];
extern const uint defaultZeemoteProfiles;
extern const KeyConfig defaultEvdevProfile[];
extern const uint defaultEvdevProfiles;
#if defined CONFIG_BASE_PS3 || defined CONFIG_BLUETOOTH_SERVER
extern const KeyConfig defaultPS3Profile[];
extern const uint defaultPS3Profiles;
#endif

void transposeKeysForPlayer(KeyConfig::KeyArray &key, uint player);

// common transpose behavior
void generic2PlayerTranspose(KeyConfig::KeyArray &key, uint player, uint startCategory);
void genericMultiplayerTranspose(KeyConfig::KeyArray &key, uint player, uint startCategory);

#ifdef CONFIG_BASE_ANDROID
static constexpr KeyConfig KEY_CONFIG_ANDROID_NAV_KEYS =
{
	Input::Event::MAP_KEYBOARD,
	0,
	"Android Navigation Keys",
	{
		EMU_CONTROLS_IN_GAME_ACTIONS_ANDROID_NAV_PROFILE_INIT,

		Input::Keycode::UP,
		Input::Keycode::RIGHT,
		Input::Keycode::DOWN,
		Input::Keycode::LEFT,
	}
};
#endif

void setupVolKeysInGame();
#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
void setupVControllerVars();
#endif
void setOnScreenControls(bool on);
void updateAutoOnScreenControlVisible();
void setupVControllerPosition();
void resolveOnScreenCollisions(_2DOrigin *movedObj = nullptr);
void updateVControlImg();

}
