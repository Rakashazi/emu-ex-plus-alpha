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

#include <emuframework/EmuInput.hh>
#include <emuframework/EmuSystem.hh>
#include <imagine/input/Device.hh>
#include <string_view>

namespace EmuEx
{

class InputDeviceConfig
{
public:
	static constexpr int8_t PLAYER_MULTI = -1;

	constexpr InputDeviceConfig() = default;
	InputDeviceConfig(Input::Device &dev):
		dev{&dev},
		player_{int8_t(dev.enumId() < EmuSystem::maxPlayers ? dev.enumId() : 0)} {}

	void deleteConf(InputManager &);
	bool setICadeMode(InputManager &, bool on);
	bool iCadeMode();
	bool joystickAxesAsDpad(Input::AxisSetId);
	void setJoystickAxesAsDpad(Input::AxisSetId, bool on);
	KeyConfigDesc keyConf(const InputManager &) const;
	void setDefaultKeyConf();
	void setKeyConfName(InputManager &, std::string_view name);
	KeyConfig *mutableKeyConf(InputManager &) const;
	KeyConfig *makeMutableKeyConf(EmuApp &);
	KeyConfig *setKeyConfCopiedFromExisting(InputManager &, std::string_view name);
	void save(InputManager &);
	void setSavedConf(const InputManager &, InputDeviceSavedConfig *savedConf, bool updateKeymap = true);
	bool hasSavedConf(const InputDeviceSavedConfig &conf) const { return savedConf && *savedConf == conf; };
	bool setKey(EmuApp &, KeyInfo, MappedKeys);
	void buildKeyMap(const InputManager &);
	constexpr Input::Device &device() { return *dev; }
	void setPlayer(const InputManager &, int);
	constexpr int8_t player() const { return player_; }

protected:
	Input::Device *dev{};
	InputDeviceSavedConfig *savedConf{};
	int8_t player_{};
public:
	bool isEnabled{true};
	IG_UseMemberIf(Config::envIsAndroid, bool, shouldHandleUnboundKeys){};
};

}
