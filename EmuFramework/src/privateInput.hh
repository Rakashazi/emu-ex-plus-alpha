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

#include <imagine/input/Input.hh>
#include <imagine/input/Device.hh>
#include <imagine/util/container/VMemArray.hh>
#ifdef CONFIG_BLUETOOTH
#include <imagine/bluetooth/BluetoothInputDevScanner.hh>
#endif
#include <emuframework/TurboInput.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuInput.hh>
#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
#include <emuframework/VController.hh>
#endif
#include <vector>
#include <list>
#include <memory>

struct InputDeviceSavedConfig
{
	const KeyConfig *keyConf{};
	uint enumId = 0;
	uint8_t player = 0;
	bool enabled = true;
	uint8_t joystickAxisAsDpadBits = 0;
	#ifdef CONFIG_INPUT_ICADE
	bool iCadeMode = 0;
	#endif
	char name[MAX_INPUT_DEVICE_NAME_SIZE]{};

	constexpr InputDeviceSavedConfig() {}

	bool operator ==(InputDeviceSavedConfig const& rhs) const
	{
		return enumId == rhs.enumId && string_equal(name, rhs.name);
	}

	bool matchesDevice(const Input::Device &dev)
	{
		//logMsg("checking against device %s,%d", name, devId);
		return dev.enumId() == enumId &&
			string_equal(dev.name(), name);
	}
};

struct InputDeviceConfig
{
	static constexpr uint PLAYER_MULTI = 0xFF;
	uint8_t player = 0;
	bool enabled = true;
	Input::Device *dev{};
	InputDeviceSavedConfig *savedConf{};

	constexpr InputDeviceConfig() {}
	InputDeviceConfig(Input::Device *dev): dev{dev}
	{
		player = dev->enumId() < EmuSystem::maxPlayers ? dev->enumId() : 0;
	}
	void deleteConf();
	#ifdef CONFIG_INPUT_ICADE
	bool setICadeMode(bool on);
	bool iCadeMode();
	#endif
	uint joystickAxisAsDpadBits();
	void setJoystickAxisAsDpadBits(uint axisMask);
	const KeyConfig &keyConf();
	void setKeyConf(const KeyConfig &kConf);
	void setDefaultKeyConf();
	KeyConfig *mutableKeyConf();
	KeyConfig *makeMutableKeyConf();
	KeyConfig *setKeyConfCopiedFromExisting(const char *name);
	void save();
	void setSavedConf(InputDeviceSavedConfig *savedConf);
	bool setKey(Input::Key mapKey, const KeyCategory &cat, int keyIdx);
};

struct KeyMapping
{
	static constexpr uint maxKeyActions = 4;
	using Action = uint8_t;
	using ActionGroup = std::array<Action, maxKeyActions>;
	std::unique_ptr<ActionGroup*[]> inputDevActionTablePtr{};
	IG::VMemArray<ActionGroup> inputDevActionTable{};

	KeyMapping() {}
	void buildAll();
	void free();
	explicit operator bool() const;
};

static const int guiKeyIdxLoadGame = 0;
static const int guiKeyIdxMenu = 1;
static const int guiKeyIdxSaveState = 2;
static const int guiKeyIdxLoadState = 3;
static const int guiKeyIdxDecStateSlot = 4;
static const int guiKeyIdxIncStateSlot = 5;
static const int guiKeyIdxFastForward = 6;
static const int guiKeyIdxGameScreenshot = 7;
static const int guiKeyIdxExit = 8;
static const int guiKeyIdxToggleFastForward = 9;

static const uint VCTRL_LAYOUT_DPAD_IDX = 0,
	VCTRL_LAYOUT_CENTER_BTN_IDX = 1,
	VCTRL_LAYOUT_FACE_BTN_GAMEPAD_IDX = 2,
	VCTRL_LAYOUT_MENU_IDX = 3,
	VCTRL_LAYOUT_FF_IDX = 4,
	VCTRL_LAYOUT_L_IDX = 5,
	VCTRL_LAYOUT_R_IDX = 6;

extern TurboInput turboActions;
extern std::list<KeyConfig> customKeyConfig;
extern std::list<InputDeviceSavedConfig> savedInputDevList;
extern std::vector<InputDeviceConfig> inputDevConf;
extern KeyMapping keyMapping;
#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
SysVController &defaultVController();
extern uint pointerInputPlayer;
#endif

void processRelPtr(Input::Event e);
void commonInitInput();
void commonUpdateInput();
void updateInputDevices();

static bool customKeyConfigsContainName(const char *name)
{
	for(auto &e : customKeyConfig)
	{
		if(string_equal(e.name, name))
			return 1;
	}
	return 0;
}

VControllerLayoutPosition vControllerPixelToLayoutPos(IG::Point2D<int> pos, IG::Point2D<int> size, IG::WindowRect viewBounds);
IG::Point2D<int> vControllerLayoutToPixelPos(VControllerLayoutPosition lPos, Gfx::Viewport viewport);
void resetVControllerPositions(VController &);
void resetVControllerOptions(VController &);
void resetAllVControllerOptions(VController &);
void initVControls(VController &, Gfx::Renderer &r);

namespace EmuControls
{

#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
void setupVControllerVars(VController &);
#endif
void updateVControlImg(VController &);

}
