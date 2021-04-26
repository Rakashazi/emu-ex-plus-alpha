#pragma once

/*  This file is part of MD.emu.

	MD.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MD.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MD.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/Cheats.hh>
#include <main/Cheats.hh>
#include <vector>
#include "system.h"
unsigned decodeCheat(const char *string, uint32 &address, uint16 &data, uint16 &originalData);

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
	TextMenuItem addCode{};

	void loadCheatItems() final;
};

class EmuEditCheatView : public BaseEditCheatView
{
public:
	EmuEditCheatView(ViewAttachParams attach, MdCheat &cheat, RefreshCheatsDelegate onCheatListChanged_);

private:
	DualTextMenuItem code{};
	MdCheat *cheat{};

	const char *cheatNameString() const final;
	void renamed(const char *str) final;
};
