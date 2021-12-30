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
#include <imagine/util/string/StaticString.hh>
#ifdef CONFIG_BLUETOOTH
#include <imagine/bluetooth/BluetoothInputDevScanner.hh>
#endif
#include <emuframework/EmuInput.hh>
#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
#include <emuframework/VController.hh>
#endif
#include <memory>
#include <string>

class EmuViewController;

class InputDeviceConfig
{
public:
	static constexpr unsigned PLAYER_MULTI = 0xFF;

	constexpr InputDeviceConfig() = default;
	InputDeviceConfig(Input::Device &dev):
		dev{&dev},
		player_{(uint8_t)(dev.enumId() < EmuSystem::maxPlayers ? dev.enumId() : 0)}
	{}

	void deleteConf(InputDeviceSavedConfigContainer &);
	#ifdef CONFIG_INPUT_ICADE
	bool setICadeMode(bool on, InputDeviceSavedConfigContainer &);
	bool iCadeMode();
	#endif
	unsigned joystickAxisAsDpadBits();
	void setJoystickAxisAsDpadBits(unsigned axisMask);
	const KeyConfig &keyConf() const;
	void setKeyConf(const KeyConfig &, InputDeviceSavedConfigContainer &);
	void setDefaultKeyConf();
	KeyConfig *mutableKeyConf(KeyConfigContainer &) const;
	KeyConfig *makeMutableKeyConf(EmuApp &);
	KeyConfig *setKeyConfCopiedFromExisting(std::string_view name,
		KeyConfigContainer &, InputDeviceSavedConfigContainer &);
	void save(InputDeviceSavedConfigContainer &);
	void setSavedConf(InputDeviceSavedConfig *savedConf, bool updateKeymap = true);
	bool hasSavedConf(const InputDeviceSavedConfig &conf) const { return savedConf && *savedConf == conf; };
	bool setKey(EmuApp &, Input::Key mapKey, const KeyCategory &cat, int keyIdx);
	void buildKeyMap();
	constexpr Input::Device &device() { return *dev; }
	constexpr bool isEnabled() const { return enabled; }
	void setPlayer(int);
	constexpr uint8_t player() const { return player_; }
	constexpr void setConsumeUnboundKeys(bool on) { handleUnboundEvents = on; }
	bool shouldConsumeUnboundKeys() const { return handleUnboundEvents; }

protected:
	Input::Device *dev{};
	InputDeviceSavedConfig *savedConf{};
	uint8_t player_{};
	bool enabled{true};
	IG_UseMemberIf(Config::envIsAndroid, bool, handleUnboundEvents){};
};

struct InputDeviceData
{
	static constexpr int maxKeyActions = 4;
	using Action = uint8_t;
	using ActionGroup = std::array<Action, maxKeyActions>;

	IG::VMemArray<ActionGroup> actionTable{};
	InputDeviceConfig devConf{};
	std::string displayName{};

	InputDeviceData(Input::Device &, InputDeviceSavedConfigContainer &);
	void buildKeyMap(const Input::Device &d);
	static std::string makeDisplayName(std::string_view name, unsigned id);
};

static InputDeviceData& inputDevData(const Input::Device &d)
{
	return *d.appData<InputDeviceData>();
}

static constexpr int guiKeyIdxLoadGame = 0;
static constexpr int guiKeyIdxMenu = 1;
static constexpr int guiKeyIdxSaveState = 2;
static constexpr int guiKeyIdxLoadState = 3;
static constexpr int guiKeyIdxDecStateSlot = 4;
static constexpr int guiKeyIdxIncStateSlot = 5;
static constexpr int guiKeyIdxFastForward = 6;
static constexpr int guiKeyIdxGameScreenshot = 7;
static constexpr int guiKeyIdxExit = 8;
static constexpr int guiKeyIdxToggleFastForward = 9;
