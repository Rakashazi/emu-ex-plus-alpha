#pragma once

/*  This file is part of Lynx.emu.

	Lynx.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Lynx.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Lynx.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include "MainSystem.hh"

namespace EmuEx
{

class LynxApp final: public EmuApp
{
public:
	LynxSystem lynxSystem;

	LynxApp(ApplicationInitParams, ApplicationContext &);
	auto &system() { return lynxSystem;  }
	const auto &system() const { return lynxSystem;  }
	AssetDesc vControllerAssetDesc(KeyInfo) const;
	static std::span<const KeyCategory> keyCategories();
	static std::span<const KeyConfigDesc> defaultKeyConfigs();
	static std::string_view systemKeyCodeToString(KeyCode);
	static bool allowsTurboModifier(KeyCode);
};

using MainApp = LynxApp;

}
