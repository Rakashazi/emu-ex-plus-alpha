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

class EmuCheatsView : public BaseCheatsView
{
private:
	void loadCheatItems() final;

public:
	EmuCheatsView(ViewAttachParams attach);
};

class EmuEditCheatListView : public BaseEditCheatListView
{
private:
	TextMenuItem addGGGS{};

	void loadCheatItems() final;

public:
	EmuEditCheatListView(ViewAttachParams attach);
};

class EmuEditCheatView : public BaseEditCheatView
{
private:
	DualTextMenuItem ggCode{};
	GbcCheat *cheat{};

	void renamed(const char *str) final;

public:
	EmuEditCheatView(ViewAttachParams attach, GbcCheat &cheat, RefreshCheatsDelegate onCheatListChanged_);
};
