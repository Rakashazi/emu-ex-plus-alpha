#pragma once

/*  This file is part of GBA.emu.

	GBA.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	GBA.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with GBA.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include "MainSystem.hh"
#include "EmuCheatViews.hh"

namespace EmuEx
{

class GbaApp final: public EmuApp
{
public:
	GbaSystem gbaSystem;

	GbaApp(ApplicationInitParams, ApplicationContext &);
	auto &system() { return gbaSystem;  }
	const auto &system() const { return gbaSystem;  }
	AssetDesc vControllerAssetDesc(KeyInfo) const;
	static std::span<const KeyCategory> keyCategories();
	static std::span<const KeyConfigDesc> defaultKeyConfigs();
	static std::string_view systemKeyCodeToString(KeyCode);
	static bool allowsTurboModifier(KeyCode);

	std::unique_ptr<View> makeEditCheatsView(ViewAttachParams attach, CheatsView& view)
	{
		return std::make_unique<EditCheatsView>(attach, view);
	}

	std::unique_ptr<View> makeEditCheatView(ViewAttachParams attach, Cheat& c, BaseEditCheatsView& baseView)
	{
		return std::make_unique<EditCheatView>(attach, c, baseView);
	}
};

using MainApp = GbaApp;

}
