#pragma once

/*  This file is part of GBC.emu.

	GBC.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	GBC.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with GBC.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/Cheats.hh>
#include <main/Cheats.hh>

namespace EmuEx
{

class EmuCheatsView : public BaseCheatsView
{
public:
	EmuCheatsView(ViewAttachParams attach);

private:
	void loadCheatItems() final;
};

class EmuEditCheatListView : public BaseEditCheatListView
{
public:
	EmuEditCheatListView(ViewAttachParams attach);

private:
	TextMenuItem addGGGS{};

	void loadCheatItems() final;
};

class EmuEditCheatView : public BaseEditCheatView<EmuEditCheatView>
{
public:
	EmuEditCheatView(ViewAttachParams attach, GbcCheat &cheat, RefreshCheatsDelegate onCheatListChanged_);
	std::string_view cheatNameString() const;
	void renamed(std::string_view);

private:
	std::array<MenuItem*, 3> items;
	DualTextMenuItem ggCode{};
	GbcCheat *cheat{};
};

}
