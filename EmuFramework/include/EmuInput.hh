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

#pragma once
#include <input/interface.h>
#ifdef CONFIG_BLUETOOTH
	#include <bluetooth/BluetoothInputDevScanner.hh>
#endif
#include <TurboInput.hh>

struct KeyCategory
{
	template <size_t S>
	constexpr KeyCategory(const char *name, const char *(&keyName)[S],
			uint configOffset) :
	name(name), keyName(keyName), keys(S), configOffset(configOffset) { }

	const char *name;
	const char **keyName;
	uint keys;
	uint configOffset;
};

struct ConstKeyProfile
{
	const char *name;
	const uint *key;
};

struct KeyProfile
{
	const char *name;
	uint *key;
};

struct KeyProfileManager
{
	template <size_t S>
	constexpr KeyProfileManager(const ConstKeyProfile (&defaultProfile)[S]) :
		baseProfile(0), defaultProfiles(S), defaultProfile(defaultProfile) { }

	// constructor with no default profiles
	constexpr KeyProfileManager() :
		baseProfile(0), defaultProfiles(0), defaultProfile(0) { }

	uint baseProfile;
	uint defaultProfiles;
	const ConstKeyProfile *defaultProfile;

	bool isKeyBaseDefault(uint key, uint idx) const
	{
		if(!defaultProfiles)
			return key == 0;
		return key == defaultProfile[baseProfile].key[idx];
	}

	void loadProfile(uint *key, uint keys, int idx)
	{
		if(idx < 0 || !defaultProfiles)
			mem_zero(key, keys * sizeof(uint));
		else if((uint)idx < defaultProfiles)
		{
			//logMsg("loading profile %s, %p to %p", defaultProfile[idx].name, key, (uchar*)key + keys * sizeof(uint));
			memcpy(key, defaultProfile[idx].key, keys * sizeof(uint));
		}
		else
			bug_exit("invalid profile index %d", idx);
	}

	void loadBaseProfile(uint *key, uint keys)
	{
		loadProfile(key, keys, baseProfile);
	}
};

static const uint supportedInputDev[] =
{
#ifdef INPUT_SUPPORTS_KEYBOARD
		InputEvent::DEV_KEYBOARD,
#endif
#ifdef CONFIG_BASE_PS3
		InputEvent::DEV_PS3PAD,
#endif
#ifdef CONFIG_BLUETOOTH
		InputEvent::DEV_WIIMOTE,
		InputEvent::DEV_ICONTROLPAD,
		InputEvent::DEV_ZEEMOTE,
#endif
#ifdef CONFIG_INPUT_ICADE
		InputEvent::DEV_ICADE,
#endif
};

static const uint supportedInputDevs = sizeofArrayConst(supportedInputDev);

static uint inputDevTypeSlot(uint devType)
{
	uint slot = 0;
	switch(devType)
	{
		default: bug_branch("%d", devType); return 0;
#ifdef CONFIG_INPUT_ICADE
		case InputEvent::DEV_ICADE : slot++;
#endif
#ifdef CONFIG_BLUETOOTH
		case InputEvent::DEV_ZEEMOTE : slot++;
		case InputEvent::DEV_ICONTROLPAD : slot++;
		case InputEvent::DEV_WIIMOTE : slot++;
#endif
#ifdef CONFIG_BASE_PS3
		case InputEvent::DEV_PS3PAD : slot++;
#endif
#ifdef INPUT_SUPPORTS_KEYBOARD
		case InputEvent::DEV_KEYBOARD : slot++;
#endif
	}
	slot--;
	assert(slot < supportedInputDevs);
	return slot;
}

namespace EmuControls
{

KeyProfileManager &profileManager(uint devType);

}

template <uint KEYS>
struct KeyConfig
{
	static const uint totalKeys = KEYS;

	uint key_[supportedInputDevs][totalKeys];

	uint *key(KeyCategory &category, uint devType)
	{
		uint idx = category.configOffset;
		assert(idx < totalKeys);
		return &key_[inputDevTypeSlot(devType)][idx];
	}

	uint *key(uint devType)
	{
		return key_[inputDevTypeSlot(devType)];
	}

	void loadProfile(uint devType, int idx)
	{
		EmuControls::profileManager(devType).loadProfile(key(devType), totalKeys, idx);
	}

	void loadBaseProfile(uint devType)
	{
		EmuControls::profileManager(devType).loadBaseProfile(key(devType), totalKeys);
	}

	void unbindCategory(KeyCategory &category, uint devType)
	{
		mem_zero(key(category, devType), category.keys * sizeof(uint));
	}
};

struct KeyMapping
{
	static const uint maxKeyActions = 4;
	typedef uchar Action;
	#ifdef INPUT_SUPPORTS_KEYBOARD
	Action keyActions[Input::Key::COUNT][maxKeyActions];
	#endif
	#ifdef CONFIG_BASE_PS3
	Action ps3Actions[Input::Ps3::COUNT][maxKeyActions];
	#endif
	#ifdef CONFIG_BLUETOOTH
	Action wiimoteActions[Input::Wiimote::COUNT][maxKeyActions];
	Action icpActions[Input::iControlPad::COUNT][maxKeyActions];
	Action zeemoteActions[Input::Zeemote::COUNT][maxKeyActions];
	#endif
	#ifdef CONFIG_INPUT_ICADE
	Action iCadeActions[Input::ICade::COUNT][maxKeyActions];
	#endif

	void mapFromDevType(uint devType, Action (*&actionMap)[maxKeyActions])
	{
		switch(devType)
		{
		#ifdef CONFIG_BLUETOOTH
			bcase InputEvent::DEV_WIIMOTE:
				actionMap = wiimoteActions;
			bcase InputEvent::DEV_ICONTROLPAD:
				actionMap = icpActions;
			bcase InputEvent::DEV_ZEEMOTE:
				actionMap = zeemoteActions;
		#endif
		#ifdef CONFIG_INPUT_ICADE
			bcase InputEvent::DEV_ICADE:
				actionMap = iCadeActions;
		#endif
		#ifdef CONFIG_BASE_PS3
			bdefault:
				actionMap = ps3Actions;
		#elif defined(INPUT_SUPPORTS_KEYBOARD)
			bdefault:
				actionMap = keyActions;
		#else
			break;
			bug_exit("invalid input device in mapFromDevType");
		#endif
		}
	}

	template <size_t S>
	void build(KeyCategory (&category)[S]);
	void buildAll();
};

#ifdef INPUT_SUPPORTS_POINTER
extern uint pointerInputPlayer;
#endif
#ifdef INPUT_SUPPORTS_KEYBOARD
extern uint keyboardInputPlayer[5];
#endif
#ifdef CONFIG_BASE_PS3
extern uint gamepadInputPlayer[5];
#endif
#ifdef CONFIG_BLUETOOTH
extern uint wiimoteInputPlayer[5];
extern uint iControlPadInputPlayer[5];
extern uint zeemoteInputPlayer[5];
#endif
#ifdef CONFIG_INPUT_ICADE
extern uint iCadeInputPlayer;
#endif

static const int guiKeyIdxLoadGame = 0;
static const int guiKeyIdxMenu = 1;
static const int guiKeyIdxSaveState = 2;
static const int guiKeyIdxLoadState = 3;
static const int guiKeyIdxFastForward = 4;
static const int guiKeyIdxGameScreenshot = 5;
static const int guiKeyIdxExit = 6;

#include <inGameActionKeys.hh>
#include <main/EmuControls.hh>

namespace EmuControls
{

extern KeyCategory category[categories];

}

void processRelPtr(const InputEvent &e);
uint mapInputToPlayer(const InputEvent &e);
void commonInitInput();
void commonUpdateInput();
extern TurboInput turboActions;
extern KeyMapping keyMapping;
void resetBaseKeyProfile(uint devType = InputEvent::DEV_NULL);
