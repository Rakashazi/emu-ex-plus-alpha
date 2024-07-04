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

struct InputDeviceSessionConfig
{
	std::string_view keyConfName;
	int8_t player{playerIndexUnset};
};

class InputDeviceConfig
{
public:
	constexpr InputDeviceConfig() = default;
	InputDeviceConfig(Input::Device &dev):
		dev{&dev} {}

	void deleteConf(InputManager&);
	void deleteSessionConf(InputManager&);
	void setICadeMode(bool on);
	bool iCadeMode();
	bool joystickAxesAsKeys(Input::AxisSetId);
	void setJoystickAxesAsKeys(Input::AxisSetId, bool on);
	InputDeviceSessionConfig sessionConfig(const InputManager&) const;
	KeyConfigDesc keyConf(const InputManager&) const;
	void setDefaultKeyConf();
	void setKeyConfName(InputManager&, std::string_view name);
	std::string_view sessionKeyConfName() const
	{
		if(sessionSavedConf)
			return sessionSavedConf->keyConfName;
		else return "Default";
	}
	void setSessionKeyConfName(InputManager&, std::string_view name);
	KeyConfig *mutableKeyConf(InputManager&) const;
	KeyConfig *makeMutableKeyConf(EmuApp&);
	KeyConfig *setKeyConfCopiedFromExisting(InputManager&, std::string_view name);
	InputDeviceSavedConfig& save(InputManager&);
	InputDeviceSavedSessionConfig& saveSession(InputManager&);
	void setSavedConf(const InputManager&, InputDeviceSavedConfig*, bool updateKeymap = true);
	void setSavedConf(const InputManager&, InputDeviceSavedSessionConfig*, bool updateKeymap = true);
	bool hasSavedConf(const InputDeviceSavedConfig& conf) const { return savedConf && *savedConf == conf; };
	bool hasSavedConf(const InputDeviceSavedSessionConfig& conf) const { return sessionSavedConf && *sessionSavedConf == conf; };
	bool setKey(EmuApp&, KeyInfo, MappedKeys);
	void buildKeyMap(const InputManager&);
	constexpr Input::Device& device() { return *dev; }
	void setSavedPlayer(InputManager&, int);
	void setSavedSessionPlayer(InputManager&, int);
	int8_t savedPlayer() const { return savedConf ? savedConf->player : int8_t(dev->enumId() < EmuSystem::maxPlayers ? dev->enumId() : 0); }
	int8_t savedSessionPlayer() const { return sessionSavedConf ? sessionSavedConf->player : playerIndexUnset; }

protected:
	Input::Device* dev{};
	InputDeviceSavedConfig* savedConf{};
	InputDeviceSavedSessionConfig* sessionSavedConf{};
public:
	bool isEnabled{true};
	ConditionalMember<Config::envIsAndroid, bool> shouldHandleUnboundKeys{};
};

}
