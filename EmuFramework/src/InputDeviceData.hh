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

#include "InputDeviceConfig.hh"
#include <imagine/input/inputDefs.hh>
#include <imagine/util/container/VMemArray.hh>
#include <imagine/util/container/ArrayList.hh>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace EmuEx
{

struct InputDeviceData
{
	static constexpr int maxKeyActions = 4;
	using ActionGroup = ZArray<KeyInfo, maxKeyActions>;

	VMemArray<ActionGroup> actionTable;
	std::vector<KeyMapping> keyCombos;
	StaticArrayList<Input::Key, 8> pushedInputKeys;
	InputDeviceConfig devConf;
	std::string displayName;

	InputDeviceData(const InputManager &, Input::Device &);
	void buildKeyMap(const InputManager &, const Input::Device &d);
	void updateInputKey(const Input::KeyEvent &);
	void addInputKey(Input::Key);
	void removeInputKey(Input::Key);
	bool keysArePushed(MappedKeys);
};

inline InputDeviceData& inputDevData(const Input::Device& d)
{
	return *d.appData<InputDeviceData>();
}

}
